/*-------------------------------------------------------------------------
 *
 * converter.c
 *		  Thai text search parser
 *
 * IDENTIFICATION
 *		thai_parser/src/converter.c
 *
 * Copyright (c) 2014, Di Warachet (zdk)
 * License: http://www.gnu.org/licenses/gpl.html GPL version 2 or higher
 *-------------------------------------------------------------------------
 */

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

