











































#include "nspr.h"

#include <stdio.h>
#include <stdlib.h>






#define ORDER_PRESERVED 1




#define FD_CACHE_SIZE 1024
#define NUM_FDS 20

int main(int argc, char **argv)
{
    int i;
    PRFileDesc *fds[NUM_FDS];
    PRFileDesc *savefds[NUM_FDS];
    int numfds = sizeof(fds)/sizeof(fds[0]);

    



    PR_SetFDCacheSize(0, FD_CACHE_SIZE);
    PR_SetFDCacheSize(0, 0);
    PR_SetFDCacheSize(0, FD_CACHE_SIZE);

    
    for (i = 0; i < numfds; i++) {
        savefds[i] = PR_NewTCPSocket();
        if (NULL == savefds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
    }
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        } 
    }

    




    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
        if (fds[i] != savefds[i]) {
            fprintf(stderr, "fd cache malfunctioned\n");
            exit(1);
        }
    }
    
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        } 
    }

    
    PR_SetFDCacheSize(0, 0);

    



    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
#ifdef ORDER_PRESERVED
        if (fds[i] != savefds[numfds-1-i]) {
            fprintf(stderr, "fd stack malfunctioned\n");
            exit(1);
        }
#else
        savefds[numfds-1-i] = fds[i];
#endif
    }
    
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        } 
    }

    



    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
        if (fds[i] != savefds[numfds-1-i]) {
            fprintf(stderr, "fd stack malfunctioned\n");
            exit(1);
        }
    }
    
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        } 
    }

    
    PR_SetFDCacheSize(0, FD_CACHE_SIZE);

    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
#ifdef ORDER_PRESERVED
        if (fds[i] != savefds[i]) {
            fprintf(stderr, "fd cache malfunctioned\n");
            exit(1);
        }
#else
        savefds[i] = fds[i];
#endif
    }
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        } 
    }

    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
        if (fds[i] != savefds[i]) {
            fprintf(stderr, "fd cache malfunctioned\n");
            exit(1);
        }
    }
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        } 
    }

    
    PR_SetFDCacheSize(0, 0);

    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
#ifdef ORDER_PRESERVED
        if (fds[i] != savefds[numfds-1-i]) {
            fprintf(stderr, "fd stack malfunctioned\n");
            exit(1);
        }
#else
        savefds[numfds-1-i];
#endif
    }
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        } 
    }

    for (i = 0; i < numfds; i++) {
        fds[i] = PR_NewTCPSocket();
        if (NULL == fds[i]) {
            fprintf(stderr, "PR_NewTCPSocket failed\n");
            exit(1);
        }
        if (fds[i] != savefds[numfds-1-i]) {
            fprintf(stderr, "fd stack malfunctioned\n");
            exit(1);
        }
    }
    for (i = 0; i < numfds; i++) {
        if (PR_Close(savefds[i]) == PR_FAILURE) {
            fprintf(stderr, "PR_Close failed\n");
            exit(1);
        } 
    }

    PR_Cleanup();
    printf("PASS\n");
    return 0;
}
