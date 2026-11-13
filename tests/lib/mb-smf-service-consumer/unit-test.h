#ifndef _MB_SMF_SC_UNIT_TEST_H_
#define _MB_SMF_SC_UNIT_TEST_H_
/*
 * License: 5G-MAG Public License (v1.0)
 * Copyright: (C) 2025 British Broadcasting Corporation
 * Author(s): David Waring <david.waring2@bbc.co.uk> 
 *
 * For full license terms please see the LICENSE file distributed with this
 * program. If this file is missing then the license can be retrieved from
 * https://drive.google.com/file/d/1cinCiA778IErENZ3JN52VFW-1ffHpx7Z/view
 */
#include <stdio.h>
#include <stdbool.h>

#define PCRE2_CODE_UNIT_WIDTH 8
#include <pcre2.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct unit_test_ctx_s {
  void *private_data;
} unit_test_ctx;

typedef struct unit_test_s {
  const char *name;
  bool (*fn)(unit_test_ctx *context);
} unit_test_t;

extern const unit_test_t *unit_tests[];

extern bool register_unit_test(const unit_test_t *test);

/** Pointer tests **/

#define UT_PTR_NULL_ACTION(A, ACT) do { \
        if (A) { \
            fprintf(stderr, "expected " #A " to be <null>, result 0x%p\n", (A)); \
            ACT; \
        } \
    } while (0)

#define UT_PTR_NULL_GOTO(A, EXIT_LABEL) UT_PTR_NULL_ACTION(A, goto EXIT_LABEL)
#define UT_PTR_NULL(A)                  UT_PTR_NULL_ACTION(A, return false)

#define UT_PTR_NOT_NULL_ACTION(A, ACT) do { \
        if (!(A)) { \
            fprintf(stderr, "expected " #A " to be not <null>, result <null>\n"); \
            ACT; \
        } \
    } while (0)

#define UT_PTR_NOT_NULL_GOTO(A, EXIT_LABEL) UT_PTR_NOT_NULL_ACTION(A, goto EXIT_LABEL)
#define UT_PTR_NOT_NULL(A)                  UT_PTR_NOT_NULL_ACTION(A, return false)

/** Boolean tests **/

#define UT_BOOL_TRUE_ACTION(A, ACT) do { \
        if (!(A)) { \
            fprintf(stderr, "expected " #A " to be true, but " #A " is false\n"); \
            ACT; \
        } \
    } while (0)

#define UT_BOOL_TRUE_GOTO(A, EXIT_LABEL) UT_BOOL_TRUE_ACTION(A, goto EXIT_LABEL)
#define UT_BOOL_TRUE(A) UT_BOOL_TRUE_ACTION(A, return false)

#define UT_BOOL_FALSE_ACTION(A, ACT) do { \
        if (!!(A)) { \
            fprintf(stderr, "expected " #A " to be false, but " #A " is true\n"); \
            ACT; \
        } \
    } while (0)

#define UT_BOOL_FALSE_GOTO(A, EXIT_LABEL) UT_BOOL_FALSE_ACTION(A, goto EXIT_LABEL)
#define UT_BOOL_FALSE(A) UT_BOOL_FALSE_ACTION(A, return false)

/** Enumeration tests **/

#define UT_ENUM_EQUAL_ACTION(A, B, ACT) do { \
        if ((A) != (B)) { \
            fprintf(stderr, "expected " #A " == " #B ", result %i != %i\n", (A), (B)); \
            ACT; \
        } \
    } while (0)

#define UT_ENUM_EQUAL_GOTO(A, B, EXIT_LABEL) UT_ENUM_EQUAL_ACTION(A, B, goto EXIT_LABEL)
#define UT_ENUM_EQUAL(A, B)                  UT_ENUM_EQUAL_ACTION(A, B, return false)

/** Integer tests **/

#define UT_INT_EQUAL_ACTION(A, B, ACT) do { \
        int a = (A); \
        int b = (B); \
        if (a != b) { \
            fprintf(stderr, "expected " #A " == " #B ", result %i != %i\n", a, b); \
            ACT; \
        } \
    } while (0)

#define UT_INT_EQUAL_GOTO(A, B, EXIT_LABEL) UT_INT_EQUAL_ACTION(A, B, goto EXIT_LABEL)
#define UT_INT_EQUAL(A, B)                  UT_INT_EQUAL_ACTION(A, B, return false)

#define UT_INT_NOT_EQUAL_ACTION(A, B, ACT) do { \
        int a = (A); \
        int b = (B); \
        if (a == b) { \
            fprintf(stderr, "expected " #A " != " #B ", result %i == %i\n", a, b); \
            ACT; \
        } \
    } while (0)

#define UT_INT_NOT_EQUAL_GOTO(A, B, EXIT_LABEL) UT_INT_NOT_EQUAL_ACTION(A, B, goto EXIT_LABEL)
#define UT_INT_NOT_EQUAL(A, B)                  UT_INT_NOT_EQUAL_ACTION(A, B, return false)

/** size_t tests **/

#define UT_SIZE_T_EQUAL_ACTION(A, B, ACT) do { \
        size_t a_size = (A); \
        size_t b_size = (B); \
        if (a_size != b_size) { \
            fprintf(stderr, "expected " #A " == " #B ", result %zu != %zu\n", a_size, b_size); \
            ACT; \
        } \
    } while (0)

#define UT_SIZE_T_EQUAL_GOTO(A, B, EXIT_LABEL) UT_SIZE_T_EQUAL_ACTION(A, B, goto EXIT_LABEL)
#define UT_SIZE_T_EQUAL(A, B)                  UT_SIZE_T_EQUAL_ACTION(A, B, return false)

#define UT_SIZE_T_NOT_EQUAL_ACTION(A, B, ACT) do { \
        size_t a_size = (A); \
        size_t b_size = (B); \
        if (a_size == b_size) { \
            fprintf(stderr, "expected " #A " != " #B ", result %zu == %zu\n", a_size, b_size); \
            ACT; \
        } \
    } while (0)

#define UT_SIZE_T_NOT_EQUAL_GOTO(A, B, EXIT_LABEL) UT_SIZE_T_NOT_EQUAL_ACTION(A, B, goto EXIT_LABEL)
#define UT_SIZE_T_NOT_EQUAL(A, B)                  UT_SIZE_T_NOT_EQUAL_ACTION(A, B, return false)

/** String tests **/

#define UT_STR_EQUAL_ACTION(A, B, ACT) do { \
        if (!(A)) { \
            fprintf(stderr, "expected " #A " == " #B ", result <null> != \"%s\"\n", (B)); \
            ACT; \
        } else if (strcmp((A), (B))) { \
            fprintf(stderr, "expected " #A " == " #B ", result \"%s\" != \"%s\"\n", (A), (B)); \
            ACT; \
        } \
    } while (0)

#define UT_STR_EQUAL_GOTO(A, B, EXIT_LABEL) UT_STR_EQUAL_ACTION(A, B, goto EXIT_LABEL)
#define UT_STR_EQUAL(A, B)                  UT_STR_EQUAL_ACTION(A, B, return false)

#define UT_STR_NULL_ACTION(A, ACT) do { \
        if (A) { \
            fprintf(stderr, "expected " #A " to be <null>, result \"%s\"\n", (A)); \
            ACT; \
        } \
    } while (0)

#define UT_STR_NULL_GOTO(A, EXIT_LABEL) UT_STR_NULL_ACTION(A, goto EXIT_LABEL)
#define UT_STR_NULL(A)                  UT_STR_NULL_ACTION(A, return false)

#define UT_STR_NOT_NULL_ACTION(A, ACT) do { \
        if (!(A)) { \
            fprintf(stderr, "expected " #A " to be not <null>, result <null>\n"); \
            ACT; \
        } \
    } while (0)

#define UT_STR_NOT_NULL_GOTO(A, EXIT_LABEL) UT_STR_NOT_NULL_ACTION(A, goto EXIT_LABEL)
#define UT_STR_NOT_NULL(A)                  UT_STR_NOT_NULL_ACTION(A, return false)

#define UT_STR_MATCHES_ACTION(STR, REGEX, ACT) do { \
        if (!(STR)) { \
            fprintf(stderr, "expected " #STR " to match regex " #REGEX ", cannot match <null> string\n"); \
            ACT; \
        } \
        PCRE2_SPTR8 regex = (PCRE2_SPTR8)(REGEX); \
        PCRE2_SPTR8 str = (PCRE2_SPTR8)(STR); \
        int err = 0; \
        PCRE2_SIZE err_off = 0; \
        pcre2_code *re = pcre2_compile(regex, PCRE2_ZERO_TERMINATED, 0 /* options */, &err,  &err_off, NULL); \
        if (!re) { \
            PCRE2_UCHAR err_msg[1024]; \
            pcre2_get_error_message(err, err_msg, sizeof(err_msg)); \
            fprintf(stderr, "Failed to compile regex\n%s\n    /%s/\n%*s\n", err_msg, regex, (int)err_off+1, "---^"); \
            ACT; \
        } \
        if (pcre2_match(re, str, PCRE2_ZERO_TERMINATED, 0, 0, NULL, NULL) < 0) { \
            fprintf(stderr, "expected " #STR " to match regex " #REGEX ", match failed\n"); \
            ACT; \
        } \
    } while(0)

#define UT_STR_MATCHES_GOTO(STR, REGEX, EXIT_LABEL) UT_STR_MATCHES_ACTION(STR, REGEX, goto EXIT_LABEL)
#define UT_STR_MATCHES(STR, REGEX)                  UT_STR_MATCHES_ACTION(STR, REGEX, return false)

#define UT_STR_BEGINS_WITH_ACTION(A, B, ACT) do {\
        if (!(A)) { \
            fprintf(stderr, "expected " #A " to begin with " #B ", but " #A " is <null>\n"); \
            ACT; \
        } \
        if (strncmp((A), (B), strlen(B))) { \
            fprintf(stderr, "expected " #A " to begin with " #B ", but \"%s\" does not begin \"%s\"\n", (A), (B)); \
            ACT; \
        } \
    } while (0)

#define UT_STR_BEGINS_WITH_GOTO(A, B, EXIT_LABEL) UT_STR_BEGINS_WITH_ACTION(A, B, goto EXIT_LABEL)
#define UT_STR_BEGINS_WITH(A, B)                  UT_STR_BEGINS_WITH_ACTION(A, B, return false)

#define UT_STR_ENDS_WITH_ACTION(A, B, ACT) do {\
        if (!(A)) { \
            fprintf(stderr, "expected " #A " to end with " #B ", but " #A " is <null>\n"); \
            ACT; \
        } \
        size_t a_len = strlen(A); \
        size_t b_len = strlen(B); \
        if (a_len < b_len || strcmp((A)+a_len-b_len, (B))) { \
            fprintf(stderr, "expected " #A " to end with " #B ", but \"%s\" does not end \"%s\"\n", (A), (B)); \
            ACT; \
        } \
    } while (0)

#define UT_STR_ENDS_WITH_GOTO(A, B, EXIT_LABEL) UT_STR_ENDS_WITH_ACTION(A, B, goto EXIT_LABEL)
#define UT_STR_ENDS_WITH(A, B)                  UT_STR_ENDS_WITH_ACTION(A, B, return false)

/** JSON List tests **/

#define UT_JSON_LIST_SIZE_ACTION(LIST, SIZE, ACT) do { \
        size_t list_len = (LIST)->count; \
        size_t size_len = (SIZE); \
        if (list_len != size_len) { \
            fprintf(stderr, "expected " #LIST " to have " #SIZE " entries, result %zu != %zu\n", list_len, size_len); \
            ACT; \
        } \
    } while (0)

#define UT_JSON_LIST_SIZE_GOTO(LIST, SIZE, EXIT_LABEL) UT_JSON_LIST_SIZE_ACTION(LIST, SIZE, goto EXIT_LABEL)
#define UT_JSON_LIST_SIZE(LIST, SIZE)                  UT_JSON_LIST_SIZE_ACTION(LIST, SIZE, return false)

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* ifndef _MB_SMF_SC_UNIT_TEST_H_ */
