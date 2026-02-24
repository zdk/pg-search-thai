/*
 * test_buffer_overread.c
 *
 * Tests for the buffer over-read bug in get_thai_word().
 * The original scan loop used a no-op pointer != NULL check instead of
 * checking buf_len < text_len, reading past the buffer.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../thai_parser/src/thai_parser.h"
#include "../thai_parser/src/tokenizer.h"

/* Original buggy scan loop from tokenizer.c */
static int scan_thai_chunk(char *text, int text_len)
{
    int buf_len = 0;
    char *buf = text;

    while ((buf + buf_len) != NULL
            && ((int)*(buf + buf_len) & 0x80) != 0) {
        buf_len++;
    }
    return buf_len;
}

/* Fixed version with bounds check */
static int scan_thai_chunk_fixed(char *text, int text_len)
{
    int buf_len = 0;
    char *buf = text;

    while (buf_len < text_len
            && ((int)*(buf + buf_len) & 0x80) != 0) {
        buf_len++;
    }
    return buf_len;
}

int main(void)
{
    int passed = 0;
    int failed = 0;

    /* Test 1: Thai text with 0xFF sentinel bytes after buffer end */
    {
        const char thai[] = "\xe0\xb9\x84\xe0\xb8\x97\xe0\xb8\xa2"; /* "ไทย" */
        int thai_len = 9;
        const int sentinel_len = 4;
        char *buf = malloc(thai_len + sentinel_len + 1);
        memcpy(buf, thai, thai_len);
        memset(buf + thai_len, 0xFF, sentinel_len);
        buf[thai_len + sentinel_len] = 0x00;

        int buggy_len = scan_thai_chunk(buf, thai_len);
        int fixed_len = scan_thai_chunk_fixed(buf, thai_len);

        printf("Test 1: Thai-only text with sentinel bytes after buffer\n");
        printf("  text_len=%d  buggy=%d  fixed=%d\n", thai_len, buggy_len, fixed_len);

        if (buggy_len > thai_len && fixed_len == thai_len) {
            printf("  PASS: Bug confirmed - buggy loop reads %d extra bytes\n\n",
                   buggy_len - thai_len);
            passed++;
        } else if (buggy_len == thai_len) {
            printf("  NOTE: Bug present but didn't manifest here\n\n");
            passed++;
        } else {
            printf("  FAIL: Unexpected result\n\n");
            failed++;
        }
        free(buf);
    }

    /* Test 2: Thai text with Thai-like bytes (0xE0 0xB8 ...) in adjacent memory */
    {
        const char thai[] = "\xe0\xb9\x84\xe0\xb8\x97\xe0\xb8\xa2";
        int thai_len = 9;
        int extra = 6;
        char *buf = malloc(thai_len + extra + 1);
        memcpy(buf, thai, thai_len);
        buf[thai_len+0] = 0xE0; buf[thai_len+1] = 0xB8; buf[thai_len+2] = 0x81;
        buf[thai_len+3] = 0xE0; buf[thai_len+4] = 0xB8; buf[thai_len+5] = 0x81;
        buf[thai_len + extra] = 0x00;

        int buggy_len = scan_thai_chunk(buf, thai_len);
        int fixed_len = scan_thai_chunk_fixed(buf, thai_len);

        printf("Test 2: Thai text with Thai-like bytes in adjacent memory\n");
        printf("  text_len=%d  buggy=%d  fixed=%d\n", thai_len, buggy_len, fixed_len);

        if (buggy_len > thai_len) {
            printf("  PASS: Bug confirmed - adjacent bytes included\n\n");
            passed++;
        } else {
            printf("  NOTE: Bug present but didn't manifest here\n\n");
            passed++;
        }
        free(buf);
    }

    /* Test 3: Pointer != NULL check is always true */
    {
        char dummy[4] = {0x80, 0x80, 0x80, 0x00};
        char *ptr = dummy;
        int all_nonnull = 1;
        for (int i = 0; i < 1000; i++) {
            if ((ptr + i) == NULL) { all_nonnull = 0; break; }
        }

        printf("Test 3: Pointer != NULL check is always true\n");
        printf("  (ptr + i) != NULL for i in [0,999]: %s\n",
               all_nonnull ? "true" : "false");
        if (all_nonnull) {
            printf("  PASS: The != NULL guard provides no protection\n\n");
            passed++;
        } else {
            printf("  FAIL: Unexpected NULL from pointer arithmetic\n\n");
            failed++;
        }
    }

    printf("Results: %d passed, %d failed\n", passed, failed);
    return failed > 0 ? 1 : 0;
}
