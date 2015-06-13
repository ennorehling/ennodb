#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
#endif
#include "nosql.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#ifdef WIN32
#include <windows.h>
#include <io.h>
#else
#include <sys/mman.h>
#include <unistd.h>
#define _open(filename, oflag) open(filename, oflag)
#define _lseek(fd, offset, origin) lseek(fd, offset, origin)
#endif

static const char * id4 = "ENNO"; /* file magic: 0x4f4e4e45 */

#define ID4_VERSION 0x01
#define RELEASE_VERSION ID4_VERSION
#define MIN_VERSION ID4_VERSION

int get_key(db_table *pl, const char *key, db_entry *entry) {
    const void *matches[2];
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
    const void *matches[2];
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
        db_entry *match = (db_entry *)*matches;
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
    return fclose(pl->binlog);
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
#else
                munmap(logdata, (size_t)fsize);
#endif
                result = version;
            }
        }
    }
    return result;
}
