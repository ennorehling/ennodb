#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
#endif
#include <CuTest.h>
#include <critbit.h>
#include "nosql.h"
#include "cgiapp.h"
#include "mockfcgi.h"

#include <string.h>
#include <stdio.h>

#ifndef WIN32
#include <unistd.h>
#define _unlink(s) unlink(s)
#endif

static const char *binlog = "binlog.db";

static char * get_value(db_entry *cur, char *buffer, size_t size) {
    if (size > cur->size + 1) {
        memcpy(buffer, cur->data, cur->size);
        buffer[cur->size] = 0;
    }
    else {
        buffer[0] = 0;
    }
    return buffer;
}

static db_entry mk_entry(const char *str) {
    db_entry ret;
    ret.size = strlen(str);
    ret.data = (void*)str;
    return ret;
}

static void test_nosql_set_get(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    db_entry cur = mk_entry("HODOR");
    set_key(&tbl, "hodor", &cur);
    CuAssertIntEquals(tc, 404, get_key(&tbl, "invalid", &cur));
    memset(&cur, 0, sizeof(cur));
    CuAssertIntEquals(tc, 200, get_key(&tbl, "hodor", &cur));
    CuAssertStrEquals(tc, (const char *)cur.data, "HODOR");
}

static void test_nosql_update(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    db_entry cu1 = mk_entry("HODOR");
    db_entry cu2 = mk_entry("NODOR");
    set_key(&tbl, "hodor", &cu1);
    set_key(&tbl, "hodor", &cu2);
    memset(&cu2, 0, sizeof(cu2));
    CuAssertIntEquals(tc, 200, get_key(&tbl, "hodor", &cu2));
    CuAssertStrEquals(tc, (const char *)cu2.data, "NODOR");
}

static void test_nosql_idempotent(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    const char * strings = "HODOR\0HODOR";
    db_entry cu1 = mk_entry(strings);
    db_entry cu2 = mk_entry(strings + 6);
    db_entry cur;
    set_key(&tbl, "hodor", &cu1);
    set_key(&tbl, "hodor", &cu2);
    CuAssertIntEquals(tc, 200, get_key(&tbl, "hodor", &cur));
    CuAssertStrEquals(tc, (const char *)cur.data, "HODOR");
    CuAssertPtrEquals(tc, cu1.data, cur.data);
}

static void test_replay_log_multi(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    db_entry cur = mk_entry("HODOR");
    char buffer[32];

    _unlink(binlog);
    open_log(&tbl, binlog);
    set_key(&tbl, "hodor", &cur);
    cur = mk_entry("NOPE!");
    set_key(&tbl, "hodor", &cur);
    CuAssertIntEquals(tc, 0, close_log(&tbl));
    cb_clear(&tbl.trie);
    CuAssertIntEquals(tc, 0, read_log(&tbl, binlog));

    CuAssertIntEquals(tc, 0, _unlink(binlog));
    memset(&cur, 0, sizeof(cur));
    CuAssertIntEquals(tc, 200, get_key(&tbl, "hodor", &cur));
    CuAssertStrEquals(tc, "NOPE!", get_value(&cur, buffer, sizeof(buffer)));
}

static void test_replay_log(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    db_entry cur = mk_entry("HODOR");
    char buffer[64];

    _unlink(binlog);
    open_log(&tbl, binlog);
    set_key(&tbl, "hodor", &cur);
    CuAssertIntEquals(tc, 0, close_log(&tbl));
    cb_clear(&tbl.trie);
    read_log(&tbl, binlog);

    CuAssertIntEquals(tc, 0, _unlink(binlog));
    memset(&cur, 0, sizeof(cur));
    CuAssertIntEquals(tc, 200, get_key(&tbl, "hodor", &cur));
    CuAssertStrEquals(tc, "HODOR", get_value(&cur, buffer, sizeof(buffer)));
}

static void test_empty_log(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    FILE *F;
    const char *logname = "empty.db";

    _unlink(logname);
    F = fopen(logname, "w");
    fclose(F);
    cb_clear(&tbl.trie);
    CuAssertIntEquals(tc, 0, read_log(&tbl, logname));
    CuAssertIntEquals(tc, 0, _unlink(logname));
}

static void test_same_prefix(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    db_entry c1 = mk_entry("<img src='http://placekitten.com/g/200/300' />");
    db_entry c2 = mk_entry("<img src='http://placekitten.com/g/300/400' />");
    db_entry result;

    _unlink(binlog);
    set_key(&tbl, "cat", &c1);
    set_key(&tbl, "catz", &c1);

    set_key(&tbl, "catz", &c2);
    CuAssertIntEquals(tc, 200, get_key(&tbl, "catz", &result));
    CuAssertStrEquals(tc, (const char *)c2.data, (const char *)result.data);
    memset(&result, 0, sizeof(result));
    CuAssertIntEquals(tc, 200, get_key(&tbl, "cat", &result));
    CuAssertStrEquals(tc, (const char *)c1.data, (const char *)result.data);
}

static void test_nosql_list_keys(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    db_entry c1 = mk_entry("GOOD FOR YOU");
    db_entry c2 = mk_entry("HODOR!");
    struct db_cursor * cur;
    char buffer[128];
    const char *key;
    db_entry *val;

    set_key(&tbl, "mayo", &c1);
    set_key(&tbl, "hodor", &c2);
    CuAssertIntEquals(tc, 2, list_keys(&tbl, "", &cur));
    CuAssertIntEquals(tc, true, cursor_get(cur, &key, &val));
    CuAssertStrEquals(tc, "hodor", key);
    CuAssertStrEquals(tc, "HODOR!", get_value(val, buffer, sizeof(buffer)));
    cursor_get(cur, &key, &val);
    CuAssertStrEquals(tc, "mayo", key);
    CuAssertStrEquals(tc, "GOOD FOR YOU", get_value(val, buffer, sizeof(buffer)));
    cursor_get(cur, &key, &val);
    key = 0;
    CuAssertPtrEquals(tc, 0, (void *)key);
    cursor_reset(cur);
    cursor_get(cur, &key, &val);
    CuAssertStrEquals(tc, "hodor", key);
    CuAssertStrEquals(tc, "HODOR!", get_value(val, buffer, sizeof(buffer)));
    cursor_free(&cur);
    CuAssertPtrEquals(tc, 0, cur);
    CuAssertIntEquals(tc, 1, list_keys(&tbl, "ho", &cur));
    cursor_free(&cur);
}

void test_create_app(CuTest *tc) {
    struct app * app = create_app(0, NULL);
    FCGX_Request *req;
    CuAssertPtrNotNull(tc, app);
    req = FCGM_CreateRequest("", "REQUEST_METHOD=GET PATH_INFO=/k/hodor");
    app->process(app->data, req);
}

void test_accept_json(CuTest *tc) {
    struct app * app = create_app(0, NULL);
    FCGX_Request *req;
    CuAssertPtrNotNull(tc, app);
    req = FCGM_CreateRequest("", "REQUEST_METHOD=GET PATH_INFO=/k/hodor HTTP_ACCEPT=application/json");
    app->process(app->data, req);
    CuAssertStrEquals(tc, "", req->out->data);
}

void add_suite_critbit(CuSuite *suite);

int main(void) {
    CuString *output = CuStringNew();
    CuSuite *suite = CuSuiteNew();

    add_suite_critbit(suite);
    SUITE_ADD_TEST(suite, test_create_app);
    SUITE_ADD_TEST(suite, test_accept_json);
    SUITE_ADD_TEST(suite, test_nosql_set_get);
    SUITE_ADD_TEST(suite, test_nosql_idempotent);
    SUITE_ADD_TEST(suite, test_nosql_update);
    SUITE_ADD_TEST(suite, test_nosql_list_keys);
    SUITE_ADD_TEST(suite, test_replay_log);
    SUITE_ADD_TEST(suite, test_empty_log);
    SUITE_ADD_TEST(suite, test_replay_log_multi);
    SUITE_ADD_TEST(suite, test_same_prefix);

    CuSuiteRun(suite);
    CuSuiteSummary(suite, output);
    CuSuiteDetails(suite, output);
    printf("%s\n", output->buffer);
    return suite->failCount;
}
