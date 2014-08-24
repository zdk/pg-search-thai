/*-------------------------------------------------------------------------
 *
 * converter.c
 *		  Thai text search parser
 *
 * Copyright (c) 2014, Warachet Samtalee (zdk)
 *
 * IDENTIFICATION
 *		thai_parser/src/converter.c
 *
 *-------------------------------------------------------------------------
 */

#include <thai/thbrk.h>
#include <wchar.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <iconv.h>

#include "thai_parser.h"
#include "converter.h"

int conv_code(char* from, char* to, char* in, size_t in_len,
        char* out, size_t out_len)
{
    iconv_t conv;
    char **pin  = &in;
    char **pout = &out;

    conv = iconv_open(to,from);
    if (conv == 0) return -1;

    memset(out, 0, out_len);
    if (iconv(conv, pin , &in_len, pout, &out_len) == -1) return -1;

    iconv_close(conv);
    return 0;
}

void trans_pos(char* msg, int *pos, int pos_len)
{
    // the length of current tis 620 string
    int len = 0;
    // a tempory buffer that is used for utf-8 position calculation.
    char tmp[128];
    // the last word break postion of tis-620 string
    int last_pos = 0;

    int i = 0;
    while (i <= pos_len) {
        if (i == 0) {
            len = pos[0];
        } else if (i == pos_len) {
            len = strlen(msg);
        } else {
            len += pos[i] - last_pos;
        }
        last_pos = pos[i];

        // convert current tis 620 string to utf-8 to get the corresponding utf-8 length
        conv_code("tis620", "utf-8", msg, len, tmp, 127);
        pos[i] = strlen(tmp); //rewrite with utf-8 position
        i++;
    }
}

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