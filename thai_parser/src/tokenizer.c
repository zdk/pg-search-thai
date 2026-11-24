/*-------------------------------------------------------------------------
 *
 * tokenizer.c
 *		  Thai text search parser
 *
 * IDENTIFICATION
 *		thai_parser/src/tokenizer.c
 *
 * Copyright (c) 2014, Di Warachet (zdk)
 * License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 *-------------------------------------------------------------------------
 */

#include <thai/thbrk.h>
#include "thai_parser.h"
#include "tokenizer.h"

/* Thai letters and marks: U+0E01..U+0E3A and U+0E40..U+0E4E. */
static int is_thai(const char* text, int len)
{
    const unsigned char* s = (const unsigned char*)text;

    if (len < 3 || s[0] != 0xE0)
        return 0;
    return (s[1] == 0xB8 && s[2] >= 0x81 && s[2] <= 0xBA)
        || (s[1] == 0xB9 && s[2] >= 0x80 && s[2] <= 0x8E);
}

/* ASCII blanks and control chars, NBSP, U+2000..U+206F, U+3000. */
static int is_blank(const char* text, int len)
{
    const unsigned char* s = (const unsigned char*)text;

    if (s[0] < 0x80)
        return s[0] <= ' ' || s[0] == 0x7F;
    if (len >= 2 && s[0] == 0xC2)
        return s[1] == 0xA0;
    if (len >= 3 && s[0] == 0xE2)
        return s[1] == 0x80 || (s[1] == 0x81 && s[2] <= 0xAF);
    return len >= 3 && s[0] == 0xE3 && s[1] == 0x80 && s[2] == 0x80;
}

/* Byte length of the UTF-8 char at text, never past len. */
static int char_len(const char* text, int len)
{
    unsigned char c = (unsigned char)*text;
    int n = c < 0xC0 ? 1 : c < 0xE0 ? 2 : c < 0xF0 ? 3 : 4;

    return n < len ? n : len;
}

int th_ubrk(char* text, int* pos, int text_len)
{
    /* A Thai char is 3 bytes in UTF-8 and 1 byte in TIS-620. */
    int n = text_len / 3;
    int num, i;
    thchar_t* tis = malloc(n + 1);

    if (tis == NULL)
        return 0;

    for (i = 0; i < n; i++) {
        unsigned char* s = (unsigned char*)text + 3 * i;
        tis[i] = (s[1] == 0xB8 ? 0xA0 : 0xE0) + (s[2] & 0x3F);
    }
    tis[n] = 0;

    num = th_brk(tis, pos, n);
    for (i = 0; i < num; i++)
        pos[i] *= 3;

    free(tis);
    return num;
}

int get_thai_word(parser_ctx_t* ctx, char** token, int *token_len)
{
    int cell_num = 0;
    if (ctx->cur_id == -1) {
        ctx->buf_len = 0;
        ctx->buf     = ctx->text;
        while (is_thai(ctx->buf + ctx->buf_len, ctx->text_len - ctx->buf_len)) {
            ctx->buf_len += 3;
        }

        if (ctx->buf_len <= 0)
            return 0;

        // Allocate position array. th_brk's `n` parameter is the size of the
        // pos[] array in elements; we pass ctx->buf_len there (see th_ubrk),
        // so pos[] must hold at least that many ints. The previous formula
        // (buf_len / sizeof(wchar_t) + 1) under-sized by ~4x on long Thai
        // input, letting th_brk overrun the heap and corrupt adjacent buffers
        // — surfacing later as 'invalid byte sequence for encoding "UTF8"'.
        cell_num = ctx->buf_len + 1;
        ctx->pos = (int*)calloc(sizeof(int), cell_num);

        // word break for utf-8 text
        ctx->num    = th_ubrk(ctx->buf, ctx->pos, ctx->buf_len);
        ctx->cur_id = 0;
    }

    if (ctx->cur_id > ctx->num) {
        // no word
        *token_len     = 0;
        ctx->cur_id    = -1;
        ctx->text     += ctx->buf_len;
        ctx->text_len -= ctx->buf_len;
        free(ctx->pos);
        return 0;
    } else if (ctx->num == 0) {
        // No breakpoints inside the run -> the whole buf is one Thai word.
        // (Without this special case, the cur_id==0 branch below would read
        //  pos[0] which libthai may leave set to the TIS-620 end-position;
        //  using that as a UTF-8 byte length cuts mid-character and surfaces
        //  as 'invalid byte sequence for encoding "UTF8"' in to_tsvector and
        //  plainto_tsquery for single-word Thai inputs like "แจ็ค" or "กรุงเทพ".)
        *token_len = ctx->buf_len;
        *token     = ctx->buf;
    } else if (ctx->cur_id == 0) {
        // the first word
        *token_len = ctx->pos[0];
        *token     = ctx->buf;
    } else if (ctx->cur_id == ctx->num) {
        // the last word
        *token_len = ctx->buf_len - ctx->pos[ctx->cur_id - 1];
        *token     = ctx->buf + ctx->pos[ctx->cur_id - 1];
    } else {
        // And those middle words
        *token_len = ctx->pos[ctx->cur_id] - ctx->pos[ctx->cur_id - 1];
        *token     = ctx->buf + ctx->pos[ctx->cur_id - 1];
    }
    // move to next word
    ctx->cur_id++;
    return 'a';
}

int get_non_thai_word(parser_ctx_t* ctx, char** token, int *token_len)
{
    int len = 0;
    int blank;

    if (ctx->text_len <= 0)
        return 0;

    /* Take the run of chars that are all blank, or all not blank. */
    blank = is_blank(ctx->text, ctx->text_len);
    do {
        len += char_len(ctx->text + len, ctx->text_len - len);
    } while (len < ctx->text_len
            && !is_thai(ctx->text + len, ctx->text_len - len)
            && is_blank(ctx->text + len, ctx->text_len - len) == blank);

    *token         = ctx->text;
    *token_len     = len;
    ctx->text     += len;
    ctx->text_len -= len;
    return blank ? 'c' : 'b';
}
