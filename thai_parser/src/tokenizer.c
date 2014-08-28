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
#include "converter.h"
#include "tokenizer.h"

int th_ubrk(char* text, int* pos, int text_len)
{
    int num = 0;
    char* tis_text = calloc(text_len, sizeof(char));
    conv_code("utf-8", "tis620", text, text_len, tis_text, text_len);

    num = th_brk((const thchar_t*)tis_text, pos, text_len);
    trans_pos(tis_text, pos, num);

    free(tis_text);
    return num;
}

int get_thai_word(parser_ctx_t* ctx, char** token, int *token_len)
{
    int cell_num = 0;
    if (ctx->cur_id == -1) {
        ctx->buf_len = 0;
        ctx->buf     = ctx->text;
        while ((ctx->buf + ctx->buf_len) != NULL
                && ((int)*(ctx->buf + ctx->buf_len) & 0x80) != 0) {
            ctx->buf_len++;
        }

        if (ctx->buf_len <= 0)
            return 0;

        // Allocate position array. Use maximum length
        cell_num = ctx->buf_len / sizeof(wchar_t) + 1;
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
    int ret = 0;
    int len = 0;
    if (ctx->text_len <= 0)
        return 0;

    while (isspace(*(ctx->text + len))) {
        len++;
    }

    if (len == 0) {
        while (isalnum(*(ctx->text + len)) || ispunct(*(ctx->text + len))) {
            len++;
        }
        ret = 'b';
    } else {
        ret = 'c';
    }

    if (len > 0) {
        *token         = ctx->text;
        *token_len     = len;
        ctx->text     += len;
        ctx->text_len -= len;
        return ret;
    } else {
        return 0;
    }
}
