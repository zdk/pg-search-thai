/*-------------------------------------------------------------------------
 *
 * thai_parser.h
 *		  Thai text search parser
 *
 * Copyright (c) 2014, Warachet Samtalee (zdk)
 *
 * IDENTIFICATION
 *		thai_parser/src/thai_parser.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef THAI_PARSER_H
#define THAI_PARSER_H

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