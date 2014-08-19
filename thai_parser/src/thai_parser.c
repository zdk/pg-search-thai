/*-------------------------------------------------------------------------
 *
 * thai_parser.c
 *		  Thai text search parser
 *
 * Copyright (c) 2014, Warachet Samtalee (zdk)
 *
 * IDENTIFICATION
 *		  contrib/thai_parser/thai_parser.c
 *
 *-------------------------------------------------------------------------
 */
#include <thai/tis.h>
#include <thai/thbrk.h>
#include <thai/thwbrk.h>
#include <wchar.h>
#include <thai/thwchar.h>
#include <iconv.h>

#include "postgres.h"
#include "miscadmin.h"
#include "fmgr.h"


PG_MODULE_MAGIC;

typedef struct 
{
    char* buf;          // text to parse
    int len;            // the lenght of text
    int *pos;           // the position of breaked words
    int num;            // the number of breaked words
    int cur_id;         // the ID of current word
} parser_ctx_t;

/* copy-paste from wparser.h of tsearch2 */
typedef struct
{
    int  lexid;
    char *alias;
    char *descr;
} LexDescr;

/*
 * prototypes
 */

/**
  * convert the input string from one charset to another charset
  *
  * @param from the original charset of string
  * @param to the target charset
  * @param in the original string
  * @param in_len the original string length
  * @param out the output string buffer
  * @param out_len the output string buffer length
  * @return 0 success; -1 failure
  */
int conv_code(char* from, char* to, char* in, size_t in_len, 
        char* out, size_t out_len);

/**
  * translate the word break postion of tis620 string 
  * to corresponding position of utf-8 string
  *
  * @param msg the tis 620 string
  * @param pos the position array of tis 620 string.
  *            it also store the translated utf-8 string word break positions.
  * @param pos_len the position array length of tis 620 string
  */
void trans_pos(char* msg, int *pos, int pos_len);

/**
  * break the input utf-8 text into thai words and record the postion.
  * 
  * @param text the text need to be parsed
  * @param pos the position array for store the word postion
  * @param text_len the length of input text
  * @return the length of word break position
  */
int th_ubrk(char* text, int* pos, int text_len);

PG_FUNCTION_INFO_V1(thai_parser_start);
Datum thai_parser_start(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(thai_parser_get_token);
Datum thai_parser_get_token(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(thai_parser_end);
Datum thai_parser_end(PG_FUNCTION_ARGS);

PG_FUNCTION_INFO_V1(thai_parser_lextype);
Datum thai_parser_lextype(PG_FUNCTION_ARGS);

Datum
thai_parser_start(PG_FUNCTION_ARGS)
{
    int cell_num;
    parser_ctx_t* ctx = (parser_ctx_t*)palloc0(sizeof(parser_ctx_t));
    // save the input text and length
    ctx->buf = (char*)PG_GETARG_POINTER(0);
    ctx->len = PG_GETARG_INT32(1);

    // allocate position array. it use maximum length  
    cell_num = ctx->len / sizeof(wchar_t) + 1;
    ctx->pos = (int*)palloc0(sizeof(int) * cell_num);

    // word bread for utf-8 text
    ctx->num = th_ubrk(ctx->buf, ctx->pos, ctx->len);

    ctx->cur_id = 0;
    PG_RETURN_POINTER(ctx);
}

Datum
thai_parser_get_token(PG_FUNCTION_ARGS)
{
    parser_ctx_t* ctx = (parser_ctx_t*)PG_GETARG_POINTER(0);
    char** token = (char**)PG_GETARG_POINTER(1);
    int* len = (int*)PG_GETARG_POINTER(2);
    int type = 'a';

    if (ctx->cur_id > ctx->num)
    {
        // no word
        *len = 0;
        type = 0;
    } else if (ctx->cur_id == ctx->num)
    {
        // last word
        *len = ctx->len - ctx->pos[ctx->cur_id - 1];
        *token = ctx->buf + ctx->pos[ctx->cur_id - 1];
    } else if (ctx->cur_id == 0)
    {
        // first word
        *len = ctx->pos[0];
        *token = ctx->buf;
    } else 
    {
        // middle words
        *len = ctx->pos[ctx->cur_id] - ctx->pos[ctx->cur_id - 1];
        *token = ctx->buf + ctx->pos[ctx->cur_id - 1];
    }
    // move to next word
    ctx->cur_id++;

    PG_RETURN_INT32(type);
}

Datum
thai_parser_end(PG_FUNCTION_ARGS)
{
    parser_ctx_t* ctx = (parser_ctx_t*)PG_GETARG_POINTER(0);
    pfree(ctx->pos);
    pfree(ctx);
    PG_RETURN_VOID();
}

Datum
thai_parser_lextype(PG_FUNCTION_ARGS)
{
    LexDescr* descr = (LexDescr*)palloc(sizeof(LexDescr) * (2 + 1));
    descr[0].lexid = 97;
    descr[0].alias = pstrdup("a");
    descr[0].descr = pstrdup("word");
    descr[1].lexid = 98;
    descr[1].alias = pstrdup("b");
    descr[1].descr = pstrdup("punctuation");
    descr[26].lexid = 0;

    PG_RETURN_POINTER(descr);
}

int conv_code(char* from, char* to, char* in, size_t in_len, 
        char* out, size_t out_len)
{
    iconv_t conv;
    char **pin = &in;
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
    // tempory buffer use to calculate the length of utf-8 position
    char tmp[128];
    // last word break postion of tis 620 string 
    int last_pos = 0;

    int i = 0;
    while (i <= pos_len)
    {
        if (i == 0)
        {
            len = pos[0];
        }
        else if (i == pos_len)
        {
            len = strlen(msg);
        }
        else 
        {
            len += pos[i] - last_pos;
        }
        last_pos = pos[i];

        // convert current tis 620 string to utf-8 to get
        // the corresponding utf-8 length
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
