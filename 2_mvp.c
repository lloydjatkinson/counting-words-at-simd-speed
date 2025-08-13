#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>

static inline bool is_ws(unsigned char c)
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

    size_t file_size = (size_t)file_size_long;
    if (file_size == 0)
    {
        fclose(fp);
        printf("0\n");
        return 0;
    }

    unsigned char *data = (unsigned char *)malloc(file_size);
    if (!data)
    {
        perror("malloc");
        fclose(fp);
        return 1;
    }

    size_t nread = fread(data, 1, file_size, fp);
    fclose(fp);
    if (nread != file_size)
    {
        perror("fread");
        free(data);
        return 1;
    }

    size_t words = 0;
    bool prev_ws = true;
    for (size_t i = 0; i < nread; ++i)
    {
        unsigned char c = data[i];
        bool cur_ws = is_ws(c);
        if (!cur_ws && prev_ws)
        {
            ++words;
        }
        prev_ws = cur_ws;
    }

    free(data);
    printf("%zu\n", words);
    return 0;
}
