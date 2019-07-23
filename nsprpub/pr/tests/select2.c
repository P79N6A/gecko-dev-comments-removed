






















































#include "plgetopt.h"
#include "prttools.h"
#include "primpl.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#if defined(OS2)
#include <sys/time.h>
#endif

#define PORT 8000
#define DEFAULT_COUNT 10
PRInt32 count;



















static void Test_Result (int result)
{
	switch (result)
	{
		case PASS:
			printf ("PASS\n");
			break;
		case FAIL:
			printf ("FAIL\n");
			break;
		default:
			printf ("NOSTATUS\n");
			break;
	}
}

static void EmptyPRSelect(void)
{
    PRInt32 index = count;
    PRInt32 rv;

    for (; index--;)
        rv = PR_Select(0, NULL, NULL, NULL, PR_INTERVAL_NO_WAIT);
}

static void EmptyNativeSelect(void)
{
    PRInt32 rv;
    PRInt32 index = count;
    struct timeval timeout;

    timeout.tv_sec = timeout.tv_usec = 0;
    for (; index--;)
        rv = select(0, NULL, NULL, NULL, &timeout);
}

static void PRSelectTest(void)
{
    PRFileDesc *listenSocket;
    PRNetAddr serverAddr;

    if ( (listenSocket = PR_NewTCPSocket()) == NULL) {
        if (debug_mode) printf("\tServer error creating listen socket\n");
        return;
    }

    memset(&serverAddr, 0, sizeof(PRNetAddr));
    serverAddr.inet.family = AF_INET;
    serverAddr.inet.port = PR_htons(PORT);
    serverAddr.inet.ip = PR_htonl(INADDR_ANY);

    if ( PR_Bind(listenSocket, &serverAddr) == PR_FAILURE) {
        if (debug_mode) printf("\tServer error binding to server address\n");
        PR_Close(listenSocket);
        return;
    }

    if ( PR_Listen(listenSocket, 128) == PR_FAILURE) {
        if (debug_mode) printf("\tServer error listening to server socket\n");
        PR_Close(listenSocket);
        return;
    }
    if (debug_mode) printf("Listening on port %d\n", PORT);

    {
        PRFileDesc *newSock;
        PRNetAddr rAddr;
        PRInt32 loops = 0;
        PR_fd_set rdset;
        PRInt32 rv;
        PRInt32 bytesRead;
        char buf[11];

        loops++;

        if (debug_mode) printf("Going into accept\n"); 

        newSock = PR_Accept(listenSocket, 
                              &rAddr,
                              PR_INTERVAL_NO_TIMEOUT);

	if (newSock) {
            if (debug_mode) printf("Got connection!\n");
        } else {
	    if (debug_mode) printf("PR_Accept failed: error code %d\n", PR_GetError());
		else Test_Result (FAIL);
        }

        PR_FD_ZERO(&rdset);
        PR_FD_SET(newSock, &rdset);

        if (debug_mode) printf("Going into select \n");

        rv = PR_Select(0, &rdset, 0, 0, PR_INTERVAL_NO_TIMEOUT);

        if (debug_mode) printf("return from select is %d\n", rv);

        if (PR_FD_ISSET(newSock, &rdset)) {
            if (debug_mode) printf("I can't believe it- the socket is ready okay!\n");
        } else {
            if (debug_mode) printf("Damn; the select test failed...\n");
			else Test_Result (FAIL);
        }

        strcpy(buf, "XXXXXXXXXX");
        bytesRead = PR_Recv(newSock, buf, 10, 0, PR_INTERVAL_NO_TIMEOUT);
	buf[10] = '\0';

        if (debug_mode) printf("Recv completed with %d bytes, %s\n", bytesRead, buf);

        PR_Close(newSock);
    }

}

#if defined(XP_UNIX)

static void NativeSelectTest(void)
{
    PRFileDesc *listenSocket;
    PRNetAddr serverAddr;

    if ( (listenSocket = PR_NewTCPSocket()) == NULL) {
        if (debug_mode) printf("\tServer error creating listen socket\n");
        return;
    }

    memset(&serverAddr, 0, sizeof(PRNetAddr));
    serverAddr.inet.family = AF_INET;
    serverAddr.inet.port = PR_htons(PORT);
    serverAddr.inet.ip = PR_htonl(INADDR_ANY);

    if ( PR_Bind(listenSocket, &serverAddr) == PR_FAILURE) {
        if (debug_mode) printf("\tServer error binding to server address\n");
        PR_Close(listenSocket);
        return;
    }

    if ( PR_Listen(listenSocket, 128) == PR_FAILURE) {
        if (debug_mode) printf("\tServer error listening to server socket\n");
        PR_Close(listenSocket);
        return;
    }
    if (debug_mode) printf("Listening on port %d\n", PORT);

    {
        PRIntn osfd;
        char buf[11];
        fd_set rdset;
        PRNetAddr rAddr;
        PRFileDesc *newSock;
        struct timeval timeout;
        PRInt32 bytesRead, rv, loops = 0;

        loops++;

        if (debug_mode) printf("Going into accept\n"); 

        newSock = PR_Accept(listenSocket, &rAddr, PR_INTERVAL_NO_TIMEOUT);

	if (newSock) {
            if (debug_mode) printf("Got connection!\n");
        } else {
	    if (debug_mode) printf("PR_Accept failed: error code %d\n", PR_GetError());
		else Test_Result (FAIL);
        }

        osfd = PR_FileDesc2NativeHandle(newSock);
        FD_ZERO(&rdset);
        FD_SET(osfd, &rdset);

        if (debug_mode) printf("Going into select \n");


        timeout.tv_sec = 2; timeout.tv_usec = 0;
        rv = select(osfd + 1, &rdset, NULL, NULL, &timeout);

        if (debug_mode) printf("return from select is %d\n", rv);

        if (FD_ISSET(osfd, &rdset)) {
            if (debug_mode)
                printf("I can't believe it- the socket is ready okay!\n");
        } else {
            if (debug_mode) printf("Damn; the select test failed...\n");
			else Test_Result (FAIL);
        }

        strcpy(buf, "XXXXXXXXXX");
        bytesRead = PR_Recv(newSock, buf, 10, 0, PR_INTERVAL_NO_TIMEOUT);
	buf[10] = '\0';

        if (debug_mode) printf("Recv completed with %d bytes, %s\n", bytesRead, buf);

        PR_Close(newSock);
    }

}  

#endif 



static void Measure(void (*func)(void), const char *msg)
{
    PRIntervalTime start, stop;
    double d;
    PRInt32 tot;

    start = PR_IntervalNow();
    (*func)();
    stop = PR_IntervalNow();

    d = (double)PR_IntervalToMicroseconds(stop - start);
    tot = PR_IntervalToMilliseconds(stop-start);

    if (debug_mode) printf("%40s: %6.2f usec avg, %d msec total\n", msg, d / count, tot);
}

int main(int argc, char **argv)
{
	
	





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

    if (argc > 2) {
	count = atoi(argv[2]);
    } else {
	count = DEFAULT_COUNT;
    }

#if defined(XP_UNIX)
    Measure(NativeSelectTest, "time to call 1 element select()");
#endif
    Measure(EmptyPRSelect, "time to call Empty PR_select()");
    Measure(EmptyNativeSelect, "time to call Empty select()");
    Measure(PRSelectTest, "time to call 1 element PR_select()");

	if (!debug_mode) Test_Result (NOSTATUS);
    PR_Cleanup();


}
