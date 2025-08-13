#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <arm_neon.h>

static inline bool is_ws_scalar(unsigned char c)
{
    switch (c)
    {
    case ' ':
    case '\n':
    case '\r':
    case '\t':
    case '\v':
    case '\f':
        return true;
    default:
        return false;
    }
}

int main(int argc, char **argv)
{
    if (argc != 2)
    {
        fprintf(stderr, "usage: wc <file>\n");
        return 1;
    }

    FILE *fp = fopen(argv[1], "rb");
    if (!fp)
    {
        perror("open");
        return 1;
    }

    if (fseek(fp, 0, SEEK_END) != 0)
    {
        perror("fseek");
        fclose(fp);
        return 1;
    }
    long file_size_long = ftell(fp);
    if (file_size_long < 0)
    {
        perror("ftell");
        fclose(fp);
        return 1;
    }
    if (fseek(fp, 0, SEEK_SET) != 0)
    {
        perror("fseek");
        fclose(fp);
        return 1;
    }

    size_t nread = (size_t)file_size_long;
    if (nread == 0)
    {
        fclose(fp);
        printf("0\n");
        return 0;
    }

    unsigned char *buffer = (unsigned char *)malloc(nread);
    if (!buffer)
    {
        perror("malloc");
        fclose(fp);
        return 1;
    }

    size_t got = fread(buffer, 1, nread, fp);
    fclose(fp);
    if (got != nread)
    {
        perror("fread");
        free(buffer);
        return 1;
    }

    size_t words = 0;
    bool prev_ws = true;

    size_t i = 0;

    if (nread >= 16)
    {
        // (0) Previous-byte whitespace (defaults to whitespace for start-of-file)
        uint8x16_t prev_ws_vec = vdupq_n_u8(prev_ws ? 0xFF : 0x00);

        size_t nvec = nread & ~(size_t)15;
        for (; i < nvec; i += 16)
        {
            // (1) Load 16 bytes from buffer
            // [H , e , l , l , o ,   ,   ,   , t , h , e , r , e , ! ,   ,   ]
            uint8x16_t bytes = vld1q_u8(buffer + i);

            // (2) Per-lane whitespace masks (0xFF for ws, 0x00 for non-ws)
            uint8x16_t ws1 = vceqq_u8(bytes, vdupq_n_u8(' '));
            uint8x16_t ws2 = vceqq_u8(bytes, vdupq_n_u8('\n'));
            uint8x16_t ws3 = vceqq_u8(bytes, vdupq_n_u8('\r'));
            uint8x16_t ws4 = vceqq_u8(bytes, vdupq_n_u8('\t'));
            uint8x16_t ws5 = vceqq_u8(bytes, vdupq_n_u8('\v'));
            uint8x16_t ws6 = vceqq_u8(bytes, vdupq_n_u8('\f'));

            // (3) Combine all masks into a single mask
            // [00, 00, 00, 00, 00, FF, FF, FF, 00, 00, 00, 00, 00, 00, FF, FF]
            uint8x16_t ws = vorrq_u8(ws1, ws2);
            ws = vorrq_u8(ws, ws3);
            ws = vorrq_u8(ws, ws4);
            ws = vorrq_u8(ws, ws5);
            ws = vorrq_u8(ws, ws6);

            // (4) Previous-byte whitespace for each lane
            // [FF, 00, 00, 00, 00, 00, FF, FF, FF, 00, 00, 00, 00, 00, 00, FF]
            uint8x16_t prev_ws_shifted = vextq_u8(prev_ws_vec, ws, 15);

            // (5) Word start mask: non-ws AND prev-ws (two word starts, 'H' and 't')
            // [FF, 00, 00, 00, 00, 00, 00, 00, FF, 00, 00, 00, 00, 00, 00, 00]
            uint8x16_t non_ws = vmvnq_u8(ws);
            uint8x16_t start_mask = vandq_u8(non_ws, prev_ws_shifted);

            // (6) Convert lanes to 1 by shifting right by 7 bits
            // [1 , 0 , 0 , 0 , 0 , 0 , 0 , 0 , 1 , 0 , 0 , 0 , 0 , 0 , 0 , 0 ]
            uint8x16_t ones = vshrq_n_u8(start_mask, 7); // 2 ones

            // (7) Sum all lanes
            words += (size_t)vaddvq_u8(ones); // 2

            // (8) Carry state for the next iteration
            prev_ws_vec = ws;
        }

        if (i > 0)
        {
            prev_ws = is_ws_scalar(buffer[i - 1]);
        }
    }

    for (; i < nread; ++i)
    {
        unsigned char c = buffer[i];
        bool cur_ws = is_ws_scalar(c);
        if (!cur_ws && prev_ws)
        {
            ++words;
        }
        prev_ws = cur_ws;
    }

    free(buffer);
    printf("%zu\n", words);
    return 0;
}
