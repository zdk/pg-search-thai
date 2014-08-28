/*-------------------------------------------------------------------------
 *
 * thai_parser.c
 *		  Thai text search parser
 * * IDENTIFICATION
 *		thai_parser/src/thai_parser.c
 *
 * Copyright (c) 2014, Di Warachet (zdk)
 * License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 *-------------------------------------------------------------------------
 */

#include "postgres.h"
#include "fmgr.h"
#include "thai_parser.h"
#include "tokenizer.h"

PG_MODULE_MAGIC; //Ensure that it doesn't load improperly versioned object file.

/*
 * prototypes
 */

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
    parser_ctx_t* ctx = (parser_ctx_t*)palloc0(sizeof(parser_ctx_t));
    // get the input text and its length
    ctx->text     = (char*)PG_GETARG_POINTER(0);
    ctx->text_len = PG_GETARG_INT32(1);
    ctx->cur_id   = -1;
    PG_RETURN_POINTER(ctx);
}

Datum
thai_parser_get_token(PG_FUNCTION_ARGS)
{
    parser_ctx_t* ctx = (parser_ctx_t*)PG_GETARG_POINTER(0);
    char** token = (char**)PG_GETARG_POINTER(1);
    int* len = (int*)PG_GETARG_POINTER(2);
    int type = 0;
    *len = 0;
    type = get_thai_word(ctx, token, len);
    if (type == 0)
        type = get_non_thai_word(ctx, token, len);
    PG_RETURN_INT32(type);
}

Datum
thai_parser_end(PG_FUNCTION_ARGS)
{
    parser_ctx_t* ctx = (parser_ctx_t*)PG_GETARG_POINTER(0);
    pfree(ctx);
    PG_RETURN_VOID();
}

Datum
thai_parser_lextype(PG_FUNCTION_ARGS)
{
    LexDescr* descr = (LexDescr*)palloc(sizeof(LexDescr) * (2 + 1));
    descr[0].lexid  = 97;
    descr[0].alias  = pstrdup("a");
    descr[0].descr  = pstrdup("Thai word");
    descr[1].lexid  = 98;
    descr[1].alias  = pstrdup("b");
    descr[1].descr  = pstrdup("English word");
    descr[2].lexid  = 99;
    descr[2].alias  = pstrdup("c");
    descr[2].descr  = pstrdup("Space");
    descr[26].lexid = 0;

    PG_RETURN_POINTER(descr);
}
