#include <critbit.h>
#include <stdio.h>
#include <stddef.h>

typedef struct db_table {
    critbit_tree trie;
    FILE * binlog;
} db_table;

typedef struct db_entry {
    size_t size;
    void * data;
} db_entry;

int get_key(db_table *pl, const char *key, db_entry *entry);
void set_key(db_table *pl, const char *key, db_entry *entry);
int read_log(db_table *pl, const char *logfile);
int open_log(db_table *pl, const char *logfile);
int close_log(db_table *pl);
int list_keys(db_table *pl, const char *key, char *body, size_t size);

#define MAXENTRY 2048
#define MAXKEY 1024
