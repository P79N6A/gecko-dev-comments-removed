


















































#include "prerror.h"
#include "prio.h"
#include "prproces.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#define NUM_ITERATIONS 10

static char *child_argv[] = { "pipepong2", NULL };

int main()
{
    PRFileDesc *in_pipe[2];
    PRFileDesc *out_pipe[2];
    PRStatus status;
    PRProcess *process;
    PRProcessAttr *attr;
    char buf[1024];
    PRInt32 nBytes;
    PRInt32 exitCode;
    int idx;

    status = PR_CreatePipe(&in_pipe[0], &in_pipe[1]);
    if (status == PR_FAILURE) {
        fprintf(stderr, "PR_CreatePipe failed\n");
        exit(1);
    }
    status = PR_CreatePipe(&out_pipe[0], &out_pipe[1]);
    if (status == PR_FAILURE) {
        fprintf(stderr, "PR_CreatePipe failed\n");
        exit(1);
    }

    status = PR_SetFDInheritable(in_pipe[0], PR_FALSE);
    if (status == PR_FAILURE) {
        fprintf(stderr, "PR_SetFDInheritable failed\n");
        exit(1);
    }
    status = PR_SetFDInheritable(in_pipe[1], PR_TRUE);
    if (status == PR_FAILURE) {
        fprintf(stderr, "PR_SetFDInheritable failed\n");
        exit(1);
    }
    status = PR_SetFDInheritable(out_pipe[0], PR_TRUE);
    if (status == PR_FAILURE) {
        fprintf(stderr, "PR_SetFDInheritable failed\n");
        exit(1);
    }
    status = PR_SetFDInheritable(out_pipe[1], PR_FALSE);
    if (status == PR_FAILURE) {
        fprintf(stderr, "PR_SetFDInheritable failed\n");
        exit(1);
    }

    attr = PR_NewProcessAttr();
    if (attr == NULL) {
        fprintf(stderr, "PR_NewProcessAttr failed\n");
        exit(1);
    }

    status = PR_ProcessAttrSetInheritableFD(attr, out_pipe[0], "PIPE_READ");
    if (status == PR_FAILURE) {
        fprintf(stderr, "PR_ProcessAttrSetInheritableFD failed\n");
        exit(1);
    }
    status = PR_ProcessAttrSetInheritableFD(attr, in_pipe[1], "PIPE_WRITE");
    if (status == PR_FAILURE) {
        fprintf(stderr, "PR_ProcessAttrSetInheritableFD failed\n");
        exit(1);
    }

    process = PR_CreateProcess(child_argv[0], child_argv, NULL, attr);
    if (process == NULL) {
        fprintf(stderr, "PR_CreateProcess failed\n");
        exit(1);
    }
    PR_DestroyProcessAttr(attr);
    status = PR_Close(out_pipe[0]);
    if (status == PR_FAILURE) {
        fprintf(stderr, "PR_Close failed\n");
        exit(1);
    }
    status = PR_Close(in_pipe[1]);
    if (status == PR_FAILURE) {
        fprintf(stderr, "PR_Close failed\n");
        exit(1);
    }

    for (idx = 0; idx < NUM_ITERATIONS; idx++) {
        strcpy(buf, "ping");
        printf("ping process: sending \"%s\"\n", buf);
        nBytes = PR_Write(out_pipe[1], buf, 5);
        if (nBytes == -1) {
            fprintf(stderr, "PR_Write failed: (%d, %d)\n",
                    PR_GetError(), PR_GetOSError());
            exit(1);
        }
        memset(buf, 0, sizeof(buf));
        nBytes = PR_Read(in_pipe[0], buf, sizeof(buf));
        if (nBytes == -1) {
            fprintf(stderr, "PR_Read failed\n");
            exit(1);
        }
        printf("ping process: received \"%s\"\n", buf);
        if (nBytes != 5) {
            fprintf(stderr, "ping process: expected 5 bytes but got %d bytes\n",
                    nBytes);
            exit(1);
        }
        if (strcmp(buf, "pong") != 0) {
            fprintf(stderr, "ping process: expected \"pong\" but got \"%s\"\n",
                    buf);
            exit(1);
        }
    }

    status = PR_Close(in_pipe[0]);
    if (status == PR_FAILURE) {
        fprintf(stderr, "PR_Close failed\n");
        exit(1);
    }
    status = PR_Close(out_pipe[1]);
    if (status == PR_FAILURE) {
        fprintf(stderr, "PR_Close failed\n");
        exit(1);
    }
    status = PR_WaitProcess(process, &exitCode);
    if (status == PR_FAILURE) {
        fprintf(stderr, "PR_WaitProcess failed\n");
        exit(1);
    }
    if (exitCode == 0) {
        printf("PASS\n");
        return 0;
    } else {
        printf("FAIL\n");
        return 1;
    }
}
