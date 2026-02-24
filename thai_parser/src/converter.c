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

    int ret = 0;

    conv = iconv_open(to,from);
    if (conv == (iconv_t)-1) return -1;

    if (iconv(conv, pin , &in_len, pout, &out_len) == (size_t)-1) ret = -1;
    *out = '\0';

    iconv_close(conv);
    return ret;
}

void trans_pos(char* msg, int *pos, int pos_len)
{
    int len = 0;
    int last_pos = 0;
    int msg_len = strlen(msg);
    size_t tmp_size = msg_len * 3 + 1;
    char *tmp = calloc(tmp_size, sizeof(char));
    if (tmp == NULL) return;

    int i = 0;
    while (i < pos_len) {
        if (i == 0) {
            len = pos[0];
        } else {
            len += pos[i] - last_pos;
        }
        last_pos = pos[i];

        conv_code("tis620", "utf-8", msg, len, tmp, tmp_size - 1);
        pos[i] = strlen(tmp);
        i++;
    }

    free(tmp);
}

