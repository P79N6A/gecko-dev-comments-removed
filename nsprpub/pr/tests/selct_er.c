





















































#ifdef XP_BEOS
#include <stdio.h>
int main()
{
    printf( "This test is not ported to the BeOS\n" );
    return 0;
}
#else





#include "plgetopt.h"

#include "primpl.h"
#include "pprio.h"
#include "prnetdb.h"

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


PRIntn failed_already=0;
PRIntn debug_mode;

int main(int argc, char **argv)
{
    PRFileDesc *listenSock1, *listenSock2;
    PRFileDesc *badFD;
    PRUint16 listenPort1, listenPort2;
    PRNetAddr addr;
    PR_fd_set readFdSet;
    char buf[128];
    PRInt32 retVal;

	





	PLOptStatus os;
	PLOptState *opt = PL_CreateOptState(argc, argv, "d:");
	while (PL_OPT_EOL != (os = PL_GetNextOpt(opt)))
    {
		if (PL_OPT_BAD == os) continue;
        switch (opt->option)
        {
        case 'd':  
			debug_mode = 1;
            break;
         default:
            break;
        }
    }
	PL_DestroyOptState(opt);

 
	
    PR_Init(PR_USER_THREAD, PR_PRIORITY_NORMAL, 0);
    PR_STDIO_INIT();

    if (debug_mode) {
		printf("This program tests PR_Select with sockets.  Error\n");
		printf("reporting operations are tested.\n\n");
	}

    
    if ((listenSock1 = PR_NewTCPSocket()) == NULL) {
	fprintf(stderr, "Can't create a new TCP socket\n");
	failed_already=1;
	goto exit_now;
    }
    addr.inet.family = AF_INET;
    addr.inet.ip = PR_htonl(INADDR_ANY);
    addr.inet.port = PR_htons(0);
    if (PR_Bind(listenSock1, &addr) == PR_FAILURE) {
	fprintf(stderr, "Can't bind socket\n");
	failed_already=1;
	goto exit_now;
    }
    if (PR_GetSockName(listenSock1, &addr) == PR_FAILURE) {
	fprintf(stderr, "PR_GetSockName failed\n");
	failed_already=1;
	goto exit_now;
    }
    listenPort1 = PR_ntohs(addr.inet.port);
    if (PR_Listen(listenSock1, 5) == PR_FAILURE) {
	fprintf(stderr, "Can't listen on a socket\n");
	failed_already=1;
	goto exit_now;
    }

    if ((listenSock2  = PR_NewTCPSocket()) == NULL) {
	fprintf(stderr, "Can't create a new TCP socket\n");
	failed_already=1;
	goto exit_now;
    }
    addr.inet.family = AF_INET;
    addr.inet.ip = PR_htonl(INADDR_ANY);
    addr.inet.port = PR_htons(0);
    if (PR_Bind(listenSock2, &addr) == PR_FAILURE) {
	fprintf(stderr, "Can't bind socket\n");
	failed_already=1;
	goto exit_now;
    }
    if (PR_GetSockName(listenSock2, &addr) == PR_FAILURE) {
	fprintf(stderr, "PR_GetSockName failed\n");
	failed_already=1;
	goto exit_now;
    }
    listenPort2 = PR_ntohs(addr.inet.port);
    if (PR_Listen(listenSock2, 5) == PR_FAILURE) {
	fprintf(stderr, "Can't listen on a socket\n");
	failed_already=1;
	goto exit_now;
    }
    PR_snprintf(buf, sizeof(buf),
	    "The server thread is listening on ports %hu and %hu\n\n",
	    listenPort1, listenPort2);
    if (debug_mode) printf("%s", buf);

    
    PR_FD_ZERO(&readFdSet);
    PR_FD_SET(listenSock1, &readFdSet);
    PR_FD_SET(listenSock2, &readFdSet);


    
    if (debug_mode) printf("PR_Select should detect a bad file descriptor\n");
    if ((badFD = PR_NewTCPSocket()) == NULL) {
	fprintf(stderr, "Can't create a TCP socket\n");
	failed_already=1;
	goto exit_now;
    }

    PR_FD_SET(badFD, &readFdSet);
    


#if defined(XP_UNIX)
    close(PR_FileDesc2NativeHandle(badFD));
#elif defined(XP_OS2)
    soclose(PR_FileDesc2NativeHandle(badFD));
#elif defined(WIN32) || defined(WIN16)
    closesocket(PR_FileDesc2NativeHandle(badFD));
#elif defined(XP_MAC)
    _PR_MD_CLOSE_SOCKET(PR_FileDesc2NativeHandle(badFD));
#else
#error "Unknown architecture"
#endif

    retVal = PR_Select(0 , &readFdSet, NULL, NULL,
	    PR_INTERVAL_NO_TIMEOUT);
    if (retVal != -1 || PR_GetError() != PR_BAD_DESCRIPTOR_ERROR) {
	fprintf(stderr, "Failed to detect the bad fd: "
		"PR_Select returns %d\n", retVal);
	if (retVal == -1) {
	    fprintf(stderr, "Error %d, oserror %d\n", PR_GetError(),
		    PR_GetOSError());
		failed_already=1;
	}
	goto exit_now;
    }
    if (debug_mode) printf("PR_Select detected a bad fd.  Test passed.\n\n");
	PR_FD_CLR(badFD, &readFdSet);

	PR_Cleanup();
	goto exit_now;
exit_now:
	if(failed_already)	
		return 1;
	else
		return 0;

}

#endif 
