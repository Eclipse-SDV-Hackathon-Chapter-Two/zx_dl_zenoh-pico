//
// Copyright (c) 2022 ZettaScale Technology
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Apache License, Version 2.0
// which is available at https://www.apache.org/licenses/LICENSE-2.0.
//
// SPDX-License-Identifier: EPL-2.0 OR Apache-2.0
//
// Contributors:
//   ZettaScale Zenoh Team, <zenoh@zettascale.tech>

#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#undef NDEBUG
#include <assert.h>

#define URI "demo/example/**/*"
#if defined(WIN32) || defined(_WIN32) || defined(__WIN32) && !defined(__CYGWIN__)
#include <windows.h>
#define sleep(x) Sleep(x * 1000)
#else
#include <unistd.h>
#endif

#include "zenoh-pico.h"

#define SLEEP 2
#define SCOUTING_TIMEOUT "1000"

#define assert_eq(x, y)                                     \
    {                                                       \
        int l = (int)x;                                     \
        int r = (int)y;                                     \
        if (l != r) {                                       \
            printf("assert_eq failed: l=%d, r=%d\n", l, r); \
            assert(false);                                  \
        }                                                   \
    }

const char *value = "Test value";

volatile unsigned int zids = 0;
void zid_handler(const z_id_t *id, void *arg) {
    (void)(arg);
    (void)(id);
    printf("%s\n", __func__);
    zids++;
}

volatile unsigned int hellos = 0;
void hello_handler(z_owned_hello_t *hello, void *arg) {
    (void)(arg);
    printf("%s\n", __func__);
    hellos++;
    z_null(hello);
    z_drop(hello);  // validate double-drop safety: caller drops hello if it's not dropped by the handler
}

volatile unsigned int queries = 0;
void query_handler(const z_loaned_query_t *query, void *arg) {
    printf("%s\n", __func__);
    queries++;

    const z_loaned_keyexpr_t *query_ke = z_query_keyexpr(query);
    z_owned_str_t k_str;
    z_keyexpr_to_string(query_ke, &k_str);
#ifdef ZENOH_PICO
    if (z_check(k_str) == false) {
        zp_keyexpr_resolve(*(const z_loaned_session_t **)arg, z_query_keyexpr(query), &k_str);
    }
#endif

    z_view_str_t pred;
    z_query_parameters(query, &pred);
    (void)(pred);
    z_value_t payload_value = z_query_value(query);
    (void)(payload_value);
    z_query_reply_options_t _ret_qreply_opt;
    z_query_reply_options_default(&_ret_qreply_opt);
    z_query_reply(query, query_ke, (const uint8_t *)value, strlen(value), &_ret_qreply_opt);

    z_drop(z_move(k_str));
}

volatile unsigned int replies = 0;
void reply_handler(const z_loaned_reply_t *reply, void *arg) {
    printf("%s\n", __func__);
    replies++;

    if (z_reply_is_ok(reply)) {
        const z_loaned_sample_t *sample = z_reply_ok(reply);

        z_owned_str_t k_str;
        z_keyexpr_to_string(z_sample_keyexpr(sample), &k_str);
#ifdef ZENOH_PICO
        if (z_check(k_str) == false) {
            zp_keyexpr_resolve(*(const z_loaned_session_t **)arg, z_sample_keyexpr(sample), &k_str);
        }
#endif
        z_drop(z_move(k_str));
    } else {
        z_value_t _ret_zvalue = z_reply_err(reply);
        (void)(_ret_zvalue);
    }
}

volatile unsigned int datas = 0;
void data_handler(const z_loaned_sample_t *sample, void *arg) {
    printf("%s\n", __func__);
    datas++;

    z_owned_str_t k_str;
    z_keyexpr_to_string(z_sample_keyexpr(sample), &k_str);
#ifdef ZENOH_PICO
    if (z_check(k_str) == false) {
        zp_keyexpr_resolve(*(const z_loaned_session_t **)arg, z_sample_keyexpr(sample), &k_str);
    }
#endif
    z_drop(z_move(k_str));
}

int main(int argc, char **argv) {
    assert_eq(argc, 2);
    (void)(argc);
    setvbuf(stdout, NULL, _IOLBF, 1024);

#ifdef ZENOH_C
    zc_init_logger();
#endif

    z_view_keyexpr_t key_demo_example, key_demo_example_a, key_demo_example_starstar;
    z_view_keyexpr_from_string(&key_demo_example, "demo/example");
    z_view_keyexpr_from_string(&key_demo_example_a, "demo/example/a");
    z_view_keyexpr_from_string(&key_demo_example_starstar, "demo/example/**");

    _Bool _ret_bool = z_keyexpr_includes(z_loan(key_demo_example_starstar), z_loan(key_demo_example_a));
    assert(_ret_bool);
#ifdef ZENOH_PICO
    _ret_bool = zp_keyexpr_includes_null_terminated("demo/example/**", "demo/example/a");
    assert(_ret_bool);
#endif
    _ret_bool = z_keyexpr_intersects(z_loan(key_demo_example_starstar), z_loan(key_demo_example_a));
    assert(_ret_bool);
#ifdef ZENOH_PICO
    _ret_bool = zp_keyexpr_intersect_null_terminated("demo/example/**", "demo/example/a");
    assert(_ret_bool);
#endif
    _ret_bool = z_keyexpr_equals(z_loan(key_demo_example_starstar), z_loan(key_demo_example));
    assert(!_ret_bool);
#ifdef ZENOH_PICO
    _ret_bool = zp_keyexpr_equals_null_terminated("demo/example/**", "demo/example");
    assert(!_ret_bool);
#endif

    sleep(SLEEP);

    size_t keyexpr_len = strlen(URI);
    char *keyexpr_str = (char *)z_malloc(keyexpr_len + 1);
    memcpy(keyexpr_str, URI, keyexpr_len);
    keyexpr_str[keyexpr_len] = '\0';
    int8_t _ret_int8 = z_keyexpr_is_canon(keyexpr_str, keyexpr_len);
    assert(_ret_int8 < 0);

#ifdef ZENOH_PICO
    _ret_int8 = zp_keyexpr_is_canon_null_terminated(keyexpr_str);
    assert(_ret_int8 < 0);
#endif
    _ret_int8 = z_keyexpr_canonize(keyexpr_str, &keyexpr_len);
    assert_eq(_ret_int8, 0);
    assert_eq(strlen(URI), keyexpr_len);
#ifdef ZENOH_PICO
    _ret_int8 = zp_keyexpr_canonize_null_terminated(keyexpr_str);
    assert_eq(_ret_int8, 0);
    assert_eq(strlen(URI), keyexpr_len);
#endif

    printf("Ok\n");
    sleep(SLEEP);

    printf("Testing Configs...");
    z_owned_config_t _ret_config;
    z_config_new(&_ret_config);
    assert(z_check(_ret_config));
    z_drop(z_move(_ret_config));
    z_config_default(&_ret_config);
    assert(z_check(_ret_config));
#ifdef ZENOH_PICO
    _ret_int8 = zp_config_insert(z_loan_mut(_ret_config), Z_CONFIG_CONNECT_KEY, argv[1]);
    assert_eq(_ret_int8, 0);
    const char *_ret_cstr = zp_config_get(z_loan(_ret_config), Z_CONFIG_CONNECT_KEY);
    assert_eq(strlen(_ret_cstr), strlen(argv[1]));
    assert_eq(strncmp(_ret_cstr, argv[1], strlen(_ret_cstr)), 0);
#endif

    z_owned_scouting_config_t _ret_sconfig;
    z_scouting_config_default(&_ret_sconfig);
    assert(z_check(_ret_sconfig));
#ifdef ZENOH_PICO
    _ret_int8 = zp_scouting_config_insert(z_loan_mut(_ret_sconfig), Z_CONFIG_SCOUTING_TIMEOUT_KEY, SCOUTING_TIMEOUT);
    assert_eq(_ret_int8, 0);
    _ret_cstr = zp_scouting_config_get(z_loan(_ret_sconfig), Z_CONFIG_SCOUTING_TIMEOUT_KEY);
    assert_eq(strlen(_ret_cstr), strlen(SCOUTING_TIMEOUT));
    assert_eq(strncmp(_ret_cstr, SCOUTING_TIMEOUT, strlen(_ret_cstr)), 0);
#endif
    z_drop(z_move(_ret_sconfig));

    printf("Ok\n");
    sleep(SLEEP);

    printf("Testing Scouting...");
    z_scouting_config_from(&_ret_sconfig, z_loan(_ret_config));
    z_owned_closure_hello_t _ret_closure_hello;
    z_closure(&_ret_closure_hello, hello_handler, NULL, NULL);
    _ret_int8 = z_scout(z_move(_ret_sconfig), z_move(_ret_closure_hello));
    assert_eq(_ret_int8, 0);
    assert(hellos >= 1);

    uint32_t _scouting_timeout = strtoul(SCOUTING_TIMEOUT, NULL, 10);
    z_sleep_ms(_scouting_timeout);
    printf("Ok\n");
    z_sleep_s(SLEEP);

    z_owned_session_t s1;
    z_open(&s1, z_move(_ret_config));
    assert(z_check(s1));
    z_id_t _ret_zid = z_info_zid(z_loan(s1));
    printf("Session 1 with PID: 0x");
    for (unsigned long i = 0; i < sizeof(_ret_zid); i++) {
        printf("%.2X", _ret_zid.id[i]);
    }
    printf("\n");

    z_owned_closure_zid_t _ret_closure_zid;
    z_closure(&_ret_closure_zid, zid_handler, NULL, NULL);
    _ret_int8 = z_info_peers_zid(z_loan(s1), z_move(_ret_closure_zid));
    assert_eq(_ret_int8, 0);
    sleep(SLEEP);
    assert_eq(zids, 0);

    _ret_int8 = z_info_routers_zid(z_loan(s1), z_move(_ret_closure_zid));
    assert_eq(_ret_int8, 0);

    sleep(SLEEP);
    assert_eq(zids, 1);

#ifdef ZENOH_PICO
    zp_task_read_options_t _ret_read_opt;
    zp_task_read_options_default(&_ret_read_opt);
    zp_start_read_task(z_loan_mut(s1), &_ret_read_opt);
    zp_task_lease_options_t _ret_lease_opt;
    zp_task_lease_options_default(&_ret_lease_opt);
    zp_start_lease_task(z_loan_mut(s1), &_ret_lease_opt);
#endif

    sleep(SLEEP);

    z_config_default(&_ret_config);
#ifdef ZENOH_PICO
    _ret_int8 = zp_config_insert(z_loan_mut(_ret_config), Z_CONFIG_CONNECT_KEY, argv[1]);
    assert_eq(_ret_int8, 0);
    _ret_cstr = zp_config_get(z_loan(_ret_config), Z_CONFIG_CONNECT_KEY);
    assert_eq(strlen(_ret_cstr), strlen(argv[1]));
    assert_eq(strncmp(_ret_cstr, argv[1], strlen(_ret_cstr)), 0);
#endif

    z_owned_session_t s2;
    z_open(&s2, z_move(_ret_config));
    assert(z_check(s2));
    _ret_zid = z_info_zid(z_loan(s2));
    printf("Session 2 with PID: 0x");
    for (unsigned long i = 0; i < sizeof(_ret_zid); i++) {
        printf("%.2X", _ret_zid.id[i]);
    }
    printf("\n");

#ifdef ZENOH_PICO
    zp_start_read_task(z_loan_mut(s2), NULL);
    zp_start_lease_task(z_loan_mut(s2), NULL);
#endif

    sleep(SLEEP);

    const z_loaned_session_t *ls1 = z_loan(s1);
    printf("Declaring Subscriber...");
    z_owned_closure_sample_t _ret_closure_sample;
    z_closure(&_ret_closure_sample, data_handler, NULL, &ls1);
    z_subscriber_options_t _ret_sub_opt;
    z_subscriber_options_default(&_ret_sub_opt);

    z_view_keyexpr_t ke;
    z_view_keyexpr_from_string(&ke, keyexpr_str);
    z_owned_subscriber_t _ret_sub;
    _ret_int8 = z_declare_subscriber(&_ret_sub, z_loan(s2), z_loan(ke), z_move(_ret_closure_sample), &_ret_sub_opt);
    assert(_ret_int8 == _Z_RES_OK);
    printf("Ok\n");

    sleep(SLEEP);

    char s1_res[64];
    sprintf(s1_res, "%s/chunk/%d", keyexpr_str, 1);
    z_view_keyexpr_t s1_key;
    z_view_keyexpr_from_string(&s1_key, s1_res);
    z_owned_keyexpr_t _ret_expr;
    z_declare_keyexpr(&_ret_expr, z_loan(s1), z_loan(s1_key));
    assert(z_check(_ret_expr));
    printf("Ok\n");

    printf("Session Put...");
    z_put_options_t _ret_put_opt;
    z_put_options_default(&_ret_put_opt);
    _ret_put_opt.congestion_control = Z_CONGESTION_CONTROL_BLOCK;
    z_encoding_t _ret_encoding = z_encoding_default();
    (void)(_ret_encoding);
    _ret_encoding = z_encoding(Z_ENCODING_PREFIX_TEXT_PLAIN, NULL);
    _ret_put_opt.encoding = _ret_encoding;
    _ret_int8 = z_put(z_loan(s1), z_loan(_ret_expr), (const uint8_t *)value, strlen(value), &_ret_put_opt);
    assert_eq(_ret_int8, 0);
    printf("Ok\n");

    sleep(SLEEP);
    assert_eq(datas, 1);

    printf("Session delete...");
    z_delete_options_t _ret_delete_opt;
    z_delete_options_default(&_ret_delete_opt);
    _ret_delete_opt.congestion_control = Z_CONGESTION_CONTROL_BLOCK;
    _ret_int8 = z_delete(z_loan(s1), z_loan(_ret_expr), &_ret_delete_opt);
    assert_eq(_ret_int8, 0);
    printf("Ok\n");

    sleep(SLEEP);
    assert_eq(datas, 2);

    printf("Undeclaring Keyexpr...");
    _ret_int8 = z_undeclare_keyexpr(z_loan(s1), z_move(_ret_expr));
    printf(" %02x\n", _ret_int8);
    assert_eq(_ret_int8, 0);
    assert(!z_check(_ret_expr));
    printf("Ok\n");

    printf("Declaring Publisher...");
    z_publisher_options_t _ret_pub_opt;
    z_publisher_options_default(&_ret_pub_opt);
    _ret_pub_opt.congestion_control = Z_CONGESTION_CONTROL_BLOCK;
    z_owned_publisher_t _ret_pub;
    _ret_int8 = z_declare_publisher(&_ret_pub, z_loan(s1), z_loan(ke), &_ret_pub_opt);
    assert(_ret_int8 == _Z_RES_OK);
    printf("Ok\n");

    sleep(SLEEP);

    printf("Publisher Put...");
    z_publisher_put_options_t _ret_pput_opt;
    z_publisher_put_options_default(&_ret_pput_opt);
    _ret_int8 = z_publisher_put(z_loan(_ret_pub), (const uint8_t *)value, strlen(value), &_ret_pput_opt);
    assert_eq(_ret_int8, 0);
    printf("Ok\n");

    sleep(SLEEP);
    assert_eq(datas, 3);

    printf("Publisher Delete...");
    z_publisher_delete_options_t _ret_pdelete_opt;
    z_publisher_delete_options_default(&_ret_pdelete_opt);
    _ret_int8 = z_publisher_delete(z_loan(_ret_pub), &_ret_pdelete_opt);
    assert_eq(_ret_int8, 0);
    printf("Ok\n");

    sleep(SLEEP);
    assert_eq(datas, 4);

    printf("Undeclaring Publisher...");
    _ret_int8 = z_undeclare_publisher(z_move(_ret_pub));
    assert_eq(_ret_int8, 0);
    assert(!z_check(_ret_pub));
    printf("Ok\n");

    sleep(SLEEP);

    printf("Undeclaring Subscriber...");
    _ret_int8 = z_undeclare_subscriber(z_move(_ret_sub));
    assert_eq(_ret_int8, 0);
    assert(!z_check(_ret_sub));
    printf("Ok\n");

    sleep(SLEEP);

    printf("Declaring Queryable...");
    z_owned_closure_query_t _ret_closure_query;
    z_closure(&_ret_closure_query, query_handler, NULL, &ls1);
    z_queryable_options_t _ret_qle_opt;
    z_queryable_options_default(&_ret_qle_opt);
    z_owned_queryable_t qle;
    assert(z_declare_queryable(&qle, z_loan(s1), z_loan(s1_key), z_move(_ret_closure_query), &_ret_qle_opt) ==
           _Z_RES_OK);
    printf("Ok\n");

    sleep(SLEEP);

    printf("Testing Consolidations...");
    const z_loaned_session_t *ls2 = z_loan(s2);
    z_owned_closure_reply_t _ret_closure_reply;
    z_closure(&_ret_closure_reply, reply_handler, NULL, &ls2);
    z_get_options_t _ret_get_opt;
    z_get_options_default(&_ret_get_opt);
    _ret_get_opt.target = z_query_target_default();
    _ret_get_opt.consolidation = z_query_consolidation_auto();
    (void)(_ret_get_opt.consolidation);
    _ret_get_opt.consolidation = z_query_consolidation_default();
    (void)(_ret_get_opt.consolidation);
    _ret_get_opt.consolidation = z_query_consolidation_latest();
    (void)(_ret_get_opt.consolidation);
    _ret_get_opt.consolidation = z_query_consolidation_monotonic();
    (void)(_ret_get_opt.consolidation);
    _ret_get_opt.consolidation = z_query_consolidation_none();
    (void)(_ret_get_opt.consolidation);
    printf("Ok\n");

    printf("Testing Get...");
    _ret_int8 = z_get(z_loan(s2), z_loan(s1_key), "", z_move(_ret_closure_reply), &_ret_get_opt);
    assert_eq(_ret_int8, 0);
    printf("Ok\n");

    sleep(SLEEP);
    assert_eq(queries, 1);
    assert_eq(replies, 1);

    printf("Undeclaring Queryable...");
    _ret_int8 = z_undeclare_queryable(z_move(qle));
    assert_eq(_ret_int8, 0);
    printf("Ok\n");

#ifdef ZENOH_PICO
    zp_stop_read_task(z_loan_mut(s1));
    zp_stop_lease_task(z_loan_mut(s1));
#endif

    printf("Close sessions...");
    _ret_int8 = z_close(z_move(s1));
    assert_eq(_ret_int8, 0);

#ifdef ZENOH_PICO
    zp_stop_read_task(z_loan_mut(s2));
    zp_stop_lease_task(z_loan_mut(s2));
#endif
    _ret_int8 = z_close(z_move(s2));
    assert_eq(_ret_int8, 0);
    printf("Ok\n");

    sleep(SLEEP * 5);

    free(keyexpr_str);

    return 0;
}
