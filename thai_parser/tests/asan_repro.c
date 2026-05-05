/* Walk every token from the parser and flag the first invalid UTF-8 one. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "thai_parser.h"
#include "tokenizer.h"

static const char *INPUT =
    "นายขนมต้ม (อังกฤษ: Nai Khanom Tom) "
    "เป็นนักมวยไทยในสมัยกรุงศรีอยุธยา "
    "ผู้ชกชนะนักมวยพม่า 10 คนติดต่อกัน เมื่อ พ.ศ. 2317 "
    "ที่กรุงอังวะ ประเทศพม่า "
    "จนได้รับการยกย่องว่าเป็นบิดาแห่งมวยไทย";

static int valid_utf8(const unsigned char *b, int n)
{
    int i = 0;
    while (i < n) {
        unsigned char c = b[i]; int cont;
        if (c < 0x80) cont = 0;
        else if ((c & 0xE0) == 0xC0) cont = 1;
        else if ((c & 0xF0) == 0xE0) cont = 2;
        else if ((c & 0xF8) == 0xF0) cont = 3;
        else return 0;
        if (i + cont >= n) return 0;
        for (int k = 1; k <= cont; k++)
            if ((b[i + k] & 0xC0) != 0x80) return 0;
        i += 1 + cont;
    }
    return 1;
}

int main(void)
{
    parser_ctx_t ctx;
    memset(&ctx, 0, sizeof(ctx));
    ctx.text = (char*)INPUT;
    ctx.text_len = (int)strlen(INPUT);
    ctx.cur_id = -1;
    const char *base = INPUT;

    char *token = NULL;
    int token_len = 0;
    int idx = 0;
    for (;;) {
        int type = get_thai_word(&ctx, &token, &token_len);
        if (type == 0) type = get_non_thai_word(&ctx, &token, &token_len);
        if (type == 0) break;

        int ok = valid_utf8((unsigned char*)token, token_len);
        long off = token - base;
        fprintf(stderr, "[%3d] type=%c off=%-3ld len=%-3d %s | ",
                idx++, type, off, token_len, ok ? "OK" : "BAD");
        for (int j = 0; j < token_len && j < 16; j++)
            fprintf(stderr, "%02x ", (unsigned char)token[j]);
        fprintf(stderr, "\n");
        if (!ok) {
            fprintf(stderr, "first BAD at token %d, offset %ld\n", idx-1, off);
            return 1;
        }
    }
    return 0;
}
