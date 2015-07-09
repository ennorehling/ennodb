#include <critbit.h>
#include <stdio.h>
#include <stddef.h>

#define HAVE__BOOL 1

#if defined(HAVE_STDBOOL_H)
# include <stdbool.h>
#else
# if ! HAVE__BOOL
#  ifdef __cplusplus
typedef bool _Bool;
#  else
typedef unsigned char _Bool;
#  endif
# endif
# define bool _Bool
# define false 0
# define true 1
# define __bool_true_false_are_defined 1
#endif

typedef struct db_table {
    critbit_tree trie;
    FILE * binlog;
} db_table;

typedef struct db_entry {
    size_t size;
    void * data;
} db_entry;

struct db_cursor;

int get_key(db_table *pl, const char *key, db_entry *entry);
void set_key(db_table *pl, const char *key, db_entry *entry);
int read_log(db_table *pl, const char *logfile);
int open_log(db_table *pl, const char *logfile);
int close_log(db_table *pl);
int list_keys(db_table *pl, const char *prefix, struct db_cursor **cur);
char *to_json(struct db_cursor *cur, char *body, size_t size);

void cursor_free(struct db_cursor **cur);
bool cursor_get(struct db_cursor *cur, const char **key, db_entry **val);
void cursor_reset(struct db_cursor *cur);

#define MAXENTRY 2048
#define MAXKEY 1024
