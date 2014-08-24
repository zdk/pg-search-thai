/*-------------------------------------------------------------------------
 *
 * converter.h
 *		  Thai text search parser
 *
 * Copyright (c) 2014, Warachet Samtalee (zdk)
 *
 * IDENTIFICATION
 *		thai_parser/src/converter.h
 *
 *-------------------------------------------------------------------------
 */

#ifndef UTILS_H
#define UTILS_H

/**
  * convert an input string from one charset to another charset
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
  * convert the word break from the postion of tis-620 string
  * to the corresponding position of utf-8 string
  *
  * @param msg TIS-620 string
  * @param pos the position array of tis-620 string.
  *            it also stores the translated utf-8 string word break positions.
  * @param pos_len the position array length of tis-620 string
  */
void trans_pos(char* msg, int *pos, int pos_len);

/**
  * break the input utf-8 text into thai words and remember its postion.
  *
  * @param text the text that is needed to be parsed
  * @param pos the position of array in order to store the word postion
  * @param text_len a length of an input text
  * @return a length of the word break position
  */
int th_ubrk(char* text, int* pos, int text_len);

#endif


