#ifdef _MSC_VER
# define _CRT_SECURE_NO_WARNINGS
#endif
#include <CuTest.h>
#include <critbit.h>
#include "nosql.h"

#include <string.h>
#include <stdio.h>

#ifndef WIN32
#include <unistd.h>
#define _unlink(s) unlink(s)
#endif

static const char *binlog = "binlog.db";

static db_entry mk_entry(const char *str) {
    db_entry ret;
    ret.size = strlen(str)+1;
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
    CuAssertStrEquals(tc, "NOPE!", cur.data);
}

static void test_replay_log(CuTest *tc) {
    db_table tbl = { { 0 }, 0 };
    db_entry cur = mk_entry("HODOR");

    _unlink(binlog);
    open_log(&tbl, binlog);
    set_key(&tbl, "hodor", &cur);
    CuAssertIntEquals(tc, 0, close_log(&tbl));
    cb_clear(&tbl.trie);
    read_log(&tbl, binlog);

    CuAssertIntEquals(tc, 0, _unlink(binlog));
    memset(&cur, 0, sizeof(cur));
    CuAssertIntEquals(tc, 200, get_key(&tbl, "hodor", &cur));
    CuAssertStrEquals(tc, "HODOR", (const char *)cur.data);
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
	db_entry c2 = mk_entry("HODOR");
	char body[128];

	set_key(&tbl, "mayo", &c1);
	set_key(&tbl, "hodor", &c2);
	CuAssertIntEquals(tc, 2, list_keys(&tbl, "", body, sizeof(body)));
	CuAssertStrEquals(tc, "hodor: HODOR\nmayo: GOOD FOR YOU\n", body);
}

void add_suite_critbit(CuSuite *suite);

int main(void) {
    CuString *output = CuStringNew();
    CuSuite *suite = CuSuiteNew();

    add_suite_critbit(suite);
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
