#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
#endif

#ifdef WIN32
#pragma warning(push)
#pragma warning(disable: 4820 4255 4668)
#include <windows.h>
#include <io.h>
#endif

#include <sys/stat.h>
#include <sys/types.h>

#ifdef WIN32
#pragma warning(pop)
#else
#include <sys/mman.h>
#include <unistd.h>
#define _open(filename, oflag) open(filename, oflag)
#define _close(fd) close(fd)
#define _lseek(fd, offset, origin) lseek(fd, offset, origin)
#define _snprintf snprintf
#endif

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <errno.h>

#include "nosql.h"

static const char * id4 = "ENNO"; /* file magic: 0x4f4e4e45 */

#define RESULTS 16
typedef struct db_cursor {
	struct db_cursor *next;
	int index;
	const char *keys[RESULTS];
	db_entry *values[RESULTS];
} db_cursor;

#define ID4_VERSION 0x01
#define RELEASE_VERSION ID4_VERSION
#define MIN_VERSION ID4_VERSION

void cursor_free(db_cursor **cur) {
	while (*cur) {
		db_cursor *pos = *cur;
		*cur = pos->next;
		free(pos);
	}
}

void cursor_reset(db_cursor *cur) {
	if (cur) {
		cur->index = 0;
	}
}

bool cursor_get(db_cursor *cur, const char **key, db_entry **val) {
	db_cursor *pos = cur;
	int index = cur->index;
	const char *k;
	while (pos && index > RESULTS) {
		index -= RESULTS;
		pos = pos->next;
	}
	k = pos->keys[index];
	if (!k || !pos) return false;
	if (key) *key = k;
	if (val) *val = pos->values[index];
	++cur->index;
	return true;
}

char * to_json(db_cursor *cur, char *body, size_t size) {
	if (!cur->keys[0]) {
		return strncpy(body, "{}\n", size);
	}
	return body;
}

int list_keys(db_table *pl, const char *key, db_cursor **out) {
	void *matches[RESULTS];
	int total = 0, result;
	db_cursor * cur = NULL;

	do {
		result = cb_find_prefix(&pl->trie, key, strlen(key), matches, RESULTS, total);
		if (result>=1) {
			int i;
			db_cursor *cnew = malloc(sizeof(db_cursor));
			if (!cnew) {
				return -1;
			}
			cnew->next = 0;
			if (cur) {
				cur->next = cnew;
				cur = cur->next;
			}
			else {
				*out = cur = cnew;
			}
			for (i = 0; i != result; ++i) {
				cb_get_kv_ex(matches[i], (void **)(cur->values + i));
				cur->keys[i] = matches[i];
			}
			total += result;
		}
	} while (result == RESULTS);
	cur->keys[result] = 0;
	cur->index = 0;
	return total;
}

int get_key(db_table *pl, const char *key, db_entry *entry) {
    void *matches[2];
    int result;

    assert(pl && key && entry);
    result = cb_find_prefix(&pl->trie, key, strlen(key)+1, matches, 2, 0);
    printf("found %d matches for %s\n", result, key);
    if (result==1) {
        cb_get_kv(matches[0], entry, sizeof(db_entry));
        return 200;
    } else {
        return 404;
    }
}

static void insert_key(critbit_tree *trie, const char *key, size_t keylen, db_entry *entry) {
    char data[MAXENTRY+MAXKEY];
    size_t len;

    len = cb_new_kv(key, keylen, entry, sizeof(db_entry), data);
    cb_insert(trie, data, len);    
}

static void set_key_i(db_table *pl, const char *key, size_t len, db_entry *entry) {
    void *matches[2];
    int result;

    if (pl->binlog) {
        size_t size = len + 1;
        fwrite(&size, sizeof(size), 1, pl->binlog);
        fwrite(key, sizeof(char), size, pl->binlog);
        fwrite(&entry->size, sizeof(entry->size), 1, pl->binlog);
        fwrite(entry->data, sizeof(char), entry->size, pl->binlog);
        fflush(pl->binlog);
    }
    result = cb_find_prefix(&pl->trie, key, len + 1, matches, 2, 0);
    if (result > 0) {
        void *ptr;
        db_entry *match;
        cb_get_kv_ex(matches[0], &ptr);
        match = (db_entry *)ptr;
        if (match->size == entry->size && memcmp(match->data, entry->data, entry->size) == 0) {
            return;
        }
        else {
            /* replace, TODO: memory leak */
            match->data = entry->data;
            match->size = entry->size;
        }
    }
    insert_key(&pl->trie, key, len, entry);
}

void set_key(db_table *pl, const char *key, db_entry *entry) {
    size_t len = strlen(key);
    set_key_i(pl, key, len, entry);
}

int close_log(db_table *pl) {
    int result = fclose(pl->binlog);
    pl->binlog = 0;
    return result;
}

int open_log(db_table *pl, const char *logfile) {
    pl->binlog = fopen(logfile, "a+");
    if (pl->binlog) {
        fseek(pl->binlog, 0, SEEK_END);
        if (ftell(pl->binlog) == 0) {
            short version = RELEASE_VERSION;
            fwrite(id4, sizeof(char), 4, pl->binlog);
            fwrite(&version, sizeof(version), 1, pl->binlog);
            return 0;
        }
        return 0;
    }
    return errno;
}

int read_log(db_table *pl, const char *logfile) {
    int fd = _open(logfile, O_RDONLY);
    int result = 0;
    if (fd>0) {
        void *logdata;
        off_t fsize = _lseek(fd, 0, SEEK_END);
        if (fsize>0) {
#ifdef WIN32
            HANDLE fm = CreateFileMapping((HANDLE)_get_osfhandle(fd), NULL, PAGE_READONLY, 0, 0, NULL);
            logdata = (void *)MapViewOfFile(fm, FILE_MAP_READ, 0, 0, fsize);
#else
            logdata = mmap(NULL, (size_t)fsize, PROT_READ, MAP_PRIVATE, fd, 0);
#endif
            if (logdata) {
                short version = 0;
                const char *data = (const char *)logdata;
                if (fsize>=4 && memcmp(data, id4, 4) == 0) {
                    size_t header = 4 + sizeof(short);
                    memcpy(&version, data+4, sizeof(short));
                    data += header;
                }
                if (version < MIN_VERSION) {
                    printf("%s has deprecated version %d, not reading it.\n", logfile, version);
                    result = -1;
                }
                else {
                    printf("reading %u bytes from %s\n", (unsigned)fsize, logfile);
                    while (data - fsize < (const char *)logdata) {
                        db_entry entry;
                        const char *key;
                        size_t len;
                        
                        len = *(const size_t *)(const void *)data;
                        data += sizeof(size_t);
                        key = (const char *)data;
                        data += len;
                        entry.size = *(const size_t *)(const void *)data;
                        data += sizeof(size_t);
                        entry.data = memcpy(malloc(entry.size), (void *)data, entry.size);
                        data += entry.size;
                        set_key_i(pl, key, len - 1, &entry);
                    }
                }
#ifdef WIN32
                UnmapViewOfFile(logdata);
                CloseHandle(fm);
#else
                munmap(logdata, (size_t)fsize);
#endif
                result = version;
            }
        }
        result = _close(fd);
    }
    else {
        perror(logfile);
    }
    return result;
}
