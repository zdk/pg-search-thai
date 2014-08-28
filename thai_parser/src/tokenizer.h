/*-------------------------------------------------------------------------
 *
 * tokenizer.h
 *		  Thai text search parser
 *
 * IDENTIFICATION
 *		thai_parser/src/tokenizer.h
 *
 * Copyright (c) 2014, Di Warachet (zdk)
 * License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 *-------------------------------------------------------------------------
 */

#ifndef TOKENIZER_H
#define TOKENIZER_H

/**
  * break the input utf-8 text into thai words and remember its postion.
  *
  * @param text the text that is needed to be parsed
  * @param pos the position of array in order to store the word postion
  * @param text_len a length of an input text
  * @return a length of the word break position
  */
int th_ubrk(char* text, int* pos, int text_len);

/**
  * get a Thai word from an input string
  *
  * @param ctx the context for division. it includes the input information
  * @param token the word starting point
  * @param token_len the length of word
  * @return 'a' got a word;
  *         0 if not found and reach the end of buffer
  */
int get_thai_word(parser_ctx_t* ctx, char** token, int *token_len);

/**
  * get a non Thai word from an input string
  *
  * @param ctx the context for division. it includes the input information
  * @param token the segement starting point
  * @param token_len the length of word
  * @return 'b' got a English word; 'c' got a space;
  *         0 if not found and reach the end of buffer
  */
int get_non_thai_word(parser_ctx_t* ctx, char** token, int *token_len);
#endif
