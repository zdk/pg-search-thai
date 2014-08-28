/*-------------------------------------------------------------------------
 *
 * converter.h
 *		  Thai text search parser
 *
 * IDENTIFICATION
 *		thai_parser/src/converter.h
 *
 * Copyright (c) 2014, Di Warachet (zdk)
 * License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 *-------------------------------------------------------------------------
 */

#ifndef CONVERTER_H
#define CONVERTER_H

#include <iconv.h>

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

#endif
