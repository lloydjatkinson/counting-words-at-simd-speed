#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
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

// Count words in a contiguous memory range using NEON, given the starting prev-ws state
static size_t count_words_neon(const unsigned char *data, size_t len, bool prev_ws)
{
    size_t words = 0;

    size_t i = 0;

    // SIMD over 16-byte chunks
    if (len >= 16)
    {
        // (0) Previous-byte whitespace (defaults to whitespace for start-of-file)
        uint8x16_t prev_ws_vec = vdupq_n_u8(prev_ws ? 0xFF : 0x00);

        size_t nvec = len & ~(size_t)15;
        for (; i < nvec; i += 16)
        {
            // (1) Load 16 bytes from buffer
            // [H , e , l , l , o ,   ,   ,   , t , h , e , r , e , ! ,   ,   ]
            uint8x16_t bytes = vld1q_u8(data + i);

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
            prev_ws = is_ws_scalar(data[i - 1]);
        }
    }

    // Remainder bytes
    for (; i < len; ++i)
    {
        unsigned char c = data[i];
        bool cur_ws = is_ws_scalar(c);
        if (!cur_ws && prev_ws)
        {
            ++words;
        }
        prev_ws = cur_ws;
    }

    return words;
}

typedef struct ThreadArgs
{
    const unsigned char *base;
    size_t length;
    bool start_prev_ws; // derived from previous byte or SOF
    size_t result;
} ThreadArgs;

static void *worker(void *p)
{
    ThreadArgs *args = (ThreadArgs *)p;
    args->result = count_words_neon(args->base, args->length, args->start_prev_ws);
    return NULL;
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

    size_t filesize = (size_t)file_size_long;
    if (filesize == 0)
    {
        fclose(fp);
        printf("0\n");
        return 0;
    }

    unsigned char *data = (unsigned char *)malloc(filesize);
    if (!data)
    {
        perror("malloc");
        fclose(fp);
        return 1;
    }
    size_t nread = fread(data, 1, filesize, fp);
    fclose(fp);
    if (nread != filesize)
    {
        perror("fread");
        free(data);
        return 1;
    }

    // Decide number of threads: use available CPUs minus 1
    long ncpu = sysconf(_SC_NPROCESSORS_ONLN);
    if (ncpu < 1)
        ncpu = 1;

    size_t max_threads = (size_t)ncpu - 1;
    
    // Avoid spawning threads with zero work
    if (max_threads > filesize)
        max_threads = filesize;

    // Hard cap to a small static array to keep code simple
    if (max_threads > 256)
        max_threads = 256;

    // Chunk size (ceil division)
    size_t chunk = (filesize + max_threads - 1) / max_threads;

    pthread_t threads[256]; // reasonable upper bound
    ThreadArgs args[256];

    size_t launched = 0;
    for (size_t t = 0; t < max_threads; ++t)
    {
        size_t start = t * chunk;
        if (start >= filesize)
            break;
        size_t end = start + chunk;
        if (end > filesize)
            end = filesize;
        size_t len = end - start;

        // For each chunk, we need to seed with the previous-byte whitespace
        bool start_prev_ws = true; // start-of-file is "as if" preceded by whitespace
        if (start > 0)
        {
            start_prev_ws = is_ws_scalar(data[start - 1]);
        }

        args[launched].base = data + start;
        args[launched].length = len;
        args[launched].start_prev_ws = start_prev_ws;
        args[launched].result = 0;

        if (pthread_create(&threads[launched], NULL, worker, &args[launched]) != 0)
        {
            perror("pthread_create");
            free(data);
            return 1;
        }
        ++launched;
    }

    size_t total_words = 0;
    for (size_t i = 0; i < launched; ++i)
    {
        (void)pthread_join(threads[i], NULL);
        total_words += args[i].result;
    }

    free(data);
    printf("%zu\n", total_words);
    return 0;
}
