/*-------------------------------------------------------------------------
 *
 * thai_parser.h
 *		  Thai text search parser
 *
 * IDENTIFICATION
 *		thai_parser/src/thai_parser.h
 *
 * Copyright (c) 2014, Di Warachet (zdk)
 * License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 *
 *-------------------------------------------------------------------------
 */

#ifndef THAI_PARSER_H
#define THAI_PARSER_H

#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <wchar.h>

#include "postgres.h"
#include "fmgr.h"

typedef struct
{
    char* text;         // the current position of text
    int text_len;       // the length of text
    char* buf;          // A text to parse
    int buf_len;        // the length of text
    int *pos;           // The position of a breaking word
    int num;            // The number of a breaking word
    int cur_id;         // An ID of current word

} parser_ctx_t;

typedef struct
{
    int  lexid;
    char *alias;
    char *descr;
} LexDescr;

#endif
