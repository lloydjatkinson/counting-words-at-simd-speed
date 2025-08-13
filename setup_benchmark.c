#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdlib.h>
#include <unistd.h>

// Uses deterministic RNG (splitmix64) to generate a 1 GiB corpus.

#define BYTES_TOTAL (1ULL * 1024ULL * 1024ULL * 1024ULL)
#define OUT_PATH "bench.txt"
#define WRITE_CHUNK_BYTES (8ULL * 1024ULL * 1024ULL)

static const char ALPHABET[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789";

static inline uint64_t sm64(uint64_t *s) {
    uint64_t z = (*s += 0x9E3779B97F4A7C15ULL);
    z = (z ^ (z >> 30)) * 0xBF58476D1CE4E5B9ULL;
    z = (z ^ (z >> 27)) * 0x94D049BB133111EBULL;
    return z ^ (z >> 31);
}

typedef struct {
    uint64_t state, pool;
    int left;
} RNG;

static inline unsigned char next_byte(RNG *r) {
    if (r->left == 0) { r->pool = sm64(&r->state); r->left = 8; }
    unsigned char b = (unsigned char)r->pool; r->pool >>= 8; r->left--; return b;
}

static inline size_t word_len(RNG *r) {
    for (;;) { unsigned char b = next_byte(r); if (b < 240) return (size_t)(b % 30) + 1; }
}

static inline char base62(RNG *r) {
    for (;;) { unsigned char b = next_byte(r); if (b < 248) return ALPHABET[b % 62]; }
}

static inline char ws_char(RNG *r) {
    static const char WS[] = " \n\r\t\v\f"; // 6 whitespace characters
    for (;;) { unsigned char b = next_byte(r); if (b < 252) return WS[b % 6]; }
}

static int write_all(int fd, const char *data, size_t len) {
    size_t off = 0; while (off < len) { ssize_t w = write(fd, data + off, len - off);
        if (w < 0) { if (errno == EINTR) continue; return -1; } off += (size_t)w; }
    return 0;
}

int main(void) {
    int fd = open(OUT_PATH, O_WRONLY | O_CREAT | O_TRUNC, 0644);
    if (fd < 0) return 1;

    char *buf = (char *)malloc((size_t)WRITE_CHUNK_BYTES);
    if (!buf) { close(fd); return 1; }

    RNG rng = { .state = 0x243F6A8885A308D3ULL, .pool = 0, .left = 0 };
    uint64_t remaining = BYTES_TOTAL;
    size_t cap = (size_t)WRITE_CHUNK_BYTES, pos = 0;

    while (remaining > 0) {
        if (remaining == 1) {
            if (pos == cap) { if (write_all(fd, buf, pos) < 0) { free(buf); close(fd); return 1; } pos = 0; }
            buf[pos++] = base62(&rng); remaining -= 1;
        } else if (remaining <= 31) {
            size_t r = (size_t)(remaining - 1);
            if (pos + r + 1 > cap) { if (write_all(fd, buf, pos) < 0) { free(buf); close(fd); return 1; } pos = 0; }
            for (size_t i = 0; i < r; i++) buf[pos++] = base62(&rng);
            buf[pos++] = ws_char(&rng); remaining = 0;
        } else {
            size_t r = word_len(&rng);
            if (pos + r + 1 > cap) { if (write_all(fd, buf, pos) < 0) { free(buf); close(fd); return 1; } pos = 0; }
            for (size_t i = 0; i < r; i++) buf[pos++] = base62(&rng);
            buf[pos++] = ws_char(&rng); remaining -= (uint64_t)(r + 1);
        }

        if (pos >= cap || remaining == 0) {
            if (pos > 0) { if (write_all(fd, buf, pos) < 0) { free(buf); close(fd); return 1; } pos = 0; }
        }
    }

    free(buf); close(fd); return 0;
}


