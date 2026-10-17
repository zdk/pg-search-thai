/*
 * test_non_thai_word_bounds.c
 *
 * Regression test for the over-read in get_non_thai_word().
 *
 * PostgreSQL passes the parser a text datum that is NOT NUL-terminated: the
 * parser gets a pointer and a length, and the bytes after that length belong
 * to something else. get_non_thai_word() used to scan for spaces and for
 * letters/digits/punctuation without looking at text_len, so an English word
 * or number at the very end of the input ran on into those following bytes.
 * In PostgreSQL they are typically leftovers from earlier calls, so the word
 * was indexed with garbage glued on ('soup' -> 'soupcctv').
 *
 * Unlike test_buffer_overread.c, this calls the real tokenizer functions.
 * Each case puts the text at the start of a larger buffer and fills the bytes
 * after text_len with letters or spaces, i.e. exactly what the old code would
 * wrongly swallow, then checks every token stays inside text_len.
 *
 * Build (from the repository root):
 *   gcc -o tests/test_non_thai_word_bounds tests/test_non_thai_word_bounds.c \
 *       thai_parser/src/tokenizer.c thai_parser/src/converter.c -lthai
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "../thai_parser/src/thai_parser.h"
#include "../thai_parser/src/tokenizer.h"

static int failures = 0;

/*
 * Tokenize the first text_len bytes of `memory` the way thai_parser_get_token
 * does, and compare the tokens, joined with '|', against `expected`.
 */
static void check(const char *name, const char *memory, int text_len,
                  const char *expected)
{
    char *copy = strdup(memory);        /* the tokenizer takes a char * */
    parser_ctx_t ctx;
    char out[1024] = "";
    char *token = NULL;
    int token_len = 0;
    int out_of_bounds = 0;

    memset(&ctx, 0, sizeof(ctx));
    ctx.text     = copy;
    ctx.text_len = text_len;
    ctx.cur_id   = -1;

    for (;;) {
        int type = get_thai_word(&ctx, &token, &token_len);
        if (type == 0)
            type = get_non_thai_word(&ctx, &token, &token_len);
        if (type == 0)
            break;
        if (token + token_len > copy + text_len)
            out_of_bounds = 1;
        if (out[0] != '\0')
            strcat(out, "|");
        strncat(out, token, token_len);
    }

    if (out_of_bounds || ctx.text_len != 0 || strcmp(out, expected) != 0) {
        printf("FAIL: %s\n      expected [%s]\n      got      [%s]%s (text_len left: %d)\n",
               name, expected, out,
               out_of_bounds ? " -- read past the end of the text" : "",
               ctx.text_len);
        failures++;
    } else {
        printf("PASS: %s -> [%s]\n", name, out);
    }
    free(copy);
}

int main(void)
{
    /* The text is "Thai shrimp soup" (16 bytes); "cctv" after it is not part
     * of the text. The old code returned the last word as "soupcctv". */
    check("English word at the end",
          "Thai shrimp soupcctv", 16,
          "Thai| |shrimp| |soup");

    /* A number at the end, followed by digits that are not part of the text. */
    check("number at the end",
          "ซอย 12345", strlen("ซอย 12"),
          "ซอย| |12");

    /* A common real-world case: Thai text ending with a hashtag. */
    check("Thai text ending with a hashtag",
          "ไฟดับ #StreetLightinglightingu",
          strlen("ไฟดับ #StreetLighting"),
          "ไฟ|ดับ| |#StreetLighting");

    /* Trailing space at the end, followed by more spaces that are not part of
     * the text. The old code made the space token longer than the text. */
    check("space at the end",
          "CCTV      ", 5,
          "CCTV| ");

    /* Punctuation at the end is scanned by the same loop. */
    check("punctuation at the end",
          "80%%%%", 3,
          "80%");

    if (failures) {
        printf("%d case(s) FAILED\n", failures);
        return 1;
    }
    printf("All get_non_thai_word bounds checks passed\n");
    return 0;
}
