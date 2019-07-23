




















































#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_ITERATIONS 10

int main(int argc, char **argv)
{
    char buf[1024];
    size_t nBytes;
    int idx;

    for (idx = 0; idx < NUM_ITERATIONS; idx++) {
        memset(buf, 0, sizeof(buf));
        nBytes = fread(buf, 1, 5, stdin);
        fprintf(stderr, "pong process: received \"%s\"\n", buf);
        if (nBytes != 5) {
            fprintf(stderr, "pong process: expected 5 bytes but got %d bytes\n",
                    nBytes);
            exit(1);
        }
        if (strcmp(buf, "ping") != 0) {
            fprintf(stderr, "pong process: expected \"ping\" but got \"%s\"\n",
                    buf);
            exit(1);
        }

        strcpy(buf, "pong");
        fprintf(stderr, "pong process: sending \"%s\"\n", buf);
        nBytes = fwrite(buf, 1, 5, stdout);
        if (nBytes != 5) {
            fprintf(stderr, "pong process: fwrite failed\n");
            exit(1);
        }
        fflush(stdout);
    }

    return 0;
}
