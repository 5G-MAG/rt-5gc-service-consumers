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

#define UT_PTR_NULL(A) do { \
        if (A) { \
            fprintf(stderr, "expected " #A " to be <null>, result 0x%p\n", (A)); \
            return false; \
        } \
    } while (0)

#define UT_PTR_NOT_NULL(A) do { \
        if (!(A)) { \
            fprintf(stderr, "expected " #A " to be not <null>, result <null>\n"); \
            return false; \
        } \
    } while (0)

/** Boolean tests **/

#define UT_BOOL_TRUE(A) do { \
        if (!(A)) { \
            fprintf(stderr, "expected " #A " to be true, but " #A " is false\n"); \
            return false; \
        } \
    } while (0)

#define UT_BOOL_FALSE(A) do { \
        if (!!(A)) { \
            fprintf(stderr, "expected " #A " to be false, but " #A " is true\n"); \
            return false; \
        } \
    } while (0)

/** Enumeration tests **/

#define UT_ENUM_EQUAL(A, B) do { \
        if ((A) != (B)) { \
            fprintf(stderr, "expected " #A " == " #B ", result %i != %i\n", (A), (B)); \
            return false; \
        } \
    } while (0)

/** Integer tests **/

#define UT_INT_EQUAL(A, B) do { \
        int a = (A); \
        int b = (B); \
        if (a != b) { \
            fprintf(stderr, "expected " #A " == " #B ", result %i != %i\n", a, b); \
            return false; \
        } \
    } while (0)

#define UT_INT_NOT_EQUAL(A, B) do { \
        int a = (A); \
        int b = (B); \
        if (a == b) { \
            fprintf(stderr, "expected " #A " != " #B ", result %i == %i\n", a, b); \
            return false; \
        } \
    } while (0)

/** size_t tests **/

#define UT_SIZE_T_EQUAL(A, B) do { \
        size_t a_size = (A); \
        size_t b_size = (B); \
        if (a_size != b_size) { \
            fprintf(stderr, "expected " #A " == " #B ", result %zu != %zu\n", a_size, b_size); \
            return false; \
        } \
    } while (0)

#define UT_SIZE_T_NOT_EQUAL(A, B) do { \
        size_t a_size = (A); \
        size_t b_size = (B); \
        if (a_size == b_size) { \
            fprintf(stderr, "expected " #A " != " #B ", result %zu == %zu\n", a_size, b_size); \
            return false; \
        } \
    } while (0)

/** String tests **/

#define UT_STR_EQUAL(A, B) do { \
        if (!(A)) { \
            fprintf(stderr, "expected " #A " == " #B ", result <null> != \"%s\"\n", (B)); \
            return false; \
        } else if (strcmp((A), (B))) { \
            fprintf(stderr, "expected " #A " == " #B ", result \"%s\" != \"%s\"\n", (A), (B)); \
            return false; \
        } \
    } while (0)

#define UT_STR_NULL(A) do { \
        if (A) { \
            fprintf(stderr, "expected " #A " to be <null>, result \"%s\"\n", (A)); \
            return false; \
        } \
    } while (0)

#define UT_STR_NOT_NULL(A) do { \
        if (!(A)) { \
            fprintf(stderr, "expected " #A " to be not <null>, result <null>\n"); \
            return false; \
        } \
    } while (0)

#define UT_STR_MATCHES(STR, REGEX) do { \
        if (!(STR)) { \
            fprintf(stderr, "expected " #STR " to match regex " #REGEX ", cannot match <null> string\n"); \
            return false; \
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
            return false; \
        } \
        if (pcre2_match(re, str, PCRE2_ZERO_TERMINATED, 0, 0, NULL, NULL) < 0) { \
            fprintf(stderr, "expected " #STR " to match regex " #REGEX ", match failed\n"); \
            return false; \
        } \
    } while(0)

#define UT_STR_BEGINS_WITH(A, B) do {\
        if (!(A)) { \
            fprintf(stderr, "expected " #A " to begin with " #B ", but " #A " is <null>\n"); \
            return false; \
        } \
        if (strncmp((A), (B), strlen(B))) { \
            fprintf(stderr, "expected " #A " to begin with " #B ", but \"%s\" does not begin \"%s\"\n", (A), (B)); \
            return false; \
        } \
    } while (0)

#define UT_STR_ENDS_WITH(A, B) do {\
        if (!(A)) { \
            fprintf(stderr, "expected " #A " to end with " #B ", but " #A " is <null>\n"); \
            return false; \
        } \
        size_t a_len = strlen(A); \
        size_t b_len = strlen(B); \
        if (a_len < b_len || strcmp((A)+a_len-b_len, (B))) { \
            fprintf(stderr, "expected " #A " to end with " #B ", but \"%s\" does not end \"%s\"\n", (A), (B)); \
            return false; \
        } \
    } while (0)

/** JSON List tests **/

#define UT_JSON_LIST_SIZE(LIST, SIZE) do { \
        size_t list_len = (LIST)->count; \
        size_t size_len = (SIZE); \
        if (list_len != size_len) { \
            fprintf(stderr, "expected " #LIST " to have " #SIZE " entries, result %zu != %zu\n", list_len, size_len); \
            return false; \
        } \
    } while (0)

#ifdef __cplusplus
}
#endif

/* vim:ts=8:sts=4:sw=4:expandtab:
 */

#endif /* ifndef _MB_SMF_SC_UNIT_TEST_H_ */
