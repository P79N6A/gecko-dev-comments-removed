





















































#ifndef XP_UNIX

#include <stdio.h>

int main(int argc, char **argv)
{
    printf("PASS\n");
    return 0;
}

#else 

#include "nspr.h"
#include "private/pprio.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <unistd.h>

static void ClientThread(void *arg)
{
    PRFileDesc *sock;
    PRNetAddr addr;
    PRUint16 port = (PRUint16) arg;
    char buf[1024];
    PRInt32 nbytes;

    sock = PR_NewTCPSocket();
    if (NULL == sock) {
        fprintf(stderr, "PR_NewTCPSocket failed\n");
        exit(1);
    }
    if (PR_InitializeNetAddr(PR_IpAddrLoopback, port, &addr) == PR_FAILURE) {
        fprintf(stderr, "PR_InitializeNetAddr failed\n");
        exit(1);
    }
    if (PR_Connect(sock, &addr, PR_INTERVAL_NO_TIMEOUT) == PR_FAILURE) {
        fprintf(stderr, "PR_Connect failed\n");
        exit(1);
    }
    


    if (PR_Sleep(PR_SecondsToInterval(5)) == PR_FAILURE) {
        fprintf(stderr, "PR_Sleep failed\n");
        exit(1);
    }
    


    while ((nbytes = PR_Read(sock, buf, sizeof(buf))) > 0) {
        
    }
    if (-1 == nbytes) {
        fprintf(stderr, "PR_Read failed\n");
        exit(1);
    }
    if (PR_Close(sock) == PR_FAILURE) {
        fprintf(stderr, "PR_Close failed\n");
        exit(1);
    }
}

int main()
{
    PRFileDesc *listenSock;
    PRFileDesc *acceptSock;
    int osfd;
    PRThread *clientThread;
    PRNetAddr addr;
    char buf[1024];
    PRInt32 nbytes;
    PRIOVec iov;
#ifdef SYMBIAN
    int loopcount=0;
#endif

    memset(buf, 0, sizeof(buf)); 
    listenSock = PR_NewTCPSocket();
    if (NULL == listenSock) {
        fprintf(stderr, "PR_NewTCPSocket failed\n");
        exit(1);
    }
    if (PR_InitializeNetAddr(PR_IpAddrAny, 0, &addr) == PR_FAILURE) {
        fprintf(stderr, "PR_InitializeNetAddr failed\n");
        exit(1);
    }
    if (PR_Bind(listenSock, &addr) == PR_FAILURE) {
        fprintf(stderr, "PR_Bind failed\n");
        exit(1);
    }
    
    if (PR_GetSockName(listenSock, &addr) == PR_FAILURE) {
        fprintf(stderr, "PR_GetSockName failed\n");
        exit(1);
    }
    if (PR_Listen(listenSock, 5) == PR_FAILURE) {
        fprintf(stderr, "PR_Listen failed\n");
        exit(1);
    }

    


    clientThread = PR_CreateThread(PR_USER_THREAD,
            ClientThread, (void *) PR_ntohs(PR_NetAddrInetPort(&addr)),
            PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);
    if (NULL == clientThread) {
        fprintf(stderr, "PR_CreateThread failed\n");
        exit(1);
    }
    acceptSock = PR_Accept(listenSock, NULL, PR_INTERVAL_NO_TIMEOUT);
    if (NULL == acceptSock) {
        fprintf(stderr, "PR_Accept failed\n");
        exit(1);
    }
    osfd = PR_FileDesc2NativeHandle(acceptSock);
    while ((nbytes = write(osfd, buf, sizeof(buf))) != -1) {
        
#ifdef SYMBIAN
      if (loopcount++>64) break;
#endif
    }
    if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
        fprintf(stderr, "write failed\n");
        exit(1);
    }
    iov.iov_base = buf;
    iov.iov_len = 0;
    printf("calling PR_Writev with a zero-length buffer\n");
    fflush(stdout);
    nbytes = PR_Writev(acceptSock, &iov, 1, PR_INTERVAL_NO_TIMEOUT);
    if (nbytes != 0) {
        fprintf(stderr, "PR_Writev should return 0 but returns %d\n", nbytes);
        exit(1);
    }
    if (PR_Close(acceptSock) == PR_FAILURE) {
        fprintf(stderr, "PR_Close failed\n");
        exit(1);
    }
    if (PR_JoinThread(clientThread) == PR_FAILURE) {
        fprintf(stderr, "PR_JoinThread failed\n");
        exit(1);
    }

    


    clientThread = PR_CreateThread(PR_USER_THREAD,
            ClientThread, (void *) PR_ntohs(PR_NetAddrInetPort(&addr)),
            PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);
    if (NULL == clientThread) {
        fprintf(stderr, "PR_CreateThread failed\n");
        exit(1);
    }
#ifdef SYMBIAN
    loopcount = 0;
#endif
    acceptSock = PR_Accept(listenSock, NULL, PR_INTERVAL_NO_TIMEOUT);
    if (NULL == acceptSock) {
        fprintf(stderr, "PR_Accept failed\n");
        exit(1);
    }
    osfd = PR_FileDesc2NativeHandle(acceptSock);
    while ((nbytes = write(osfd, buf, sizeof(buf))) != -1) {
        
#ifdef SYMBIAN
      if (loopcount++>64) break;
#endif
    }
    if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
        fprintf(stderr, "write failed\n");
        exit(1);
    }
    printf("calling PR_Write with a zero-length buffer\n");
    fflush(stdout);
    nbytes = PR_Write(acceptSock, buf, 0);
    if (nbytes != 0) {
        fprintf(stderr, "PR_Write should return 0 but returns %d\n", nbytes);
        exit(1);
    }
    if (PR_Close(acceptSock) == PR_FAILURE) {
        fprintf(stderr, "PR_Close failed\n");
        exit(1);
    }
    if (PR_JoinThread(clientThread) == PR_FAILURE) {
        fprintf(stderr, "PR_JoinThread failed\n");
        exit(1);
    }

    


    clientThread = PR_CreateThread(PR_USER_THREAD,
            ClientThread, (void *) PR_ntohs(PR_NetAddrInetPort(&addr)),
            PR_PRIORITY_NORMAL, PR_GLOBAL_THREAD, PR_JOINABLE_THREAD, 0);
    if (NULL == clientThread) {
        fprintf(stderr, "PR_CreateThread failed\n");
        exit(1);
    }
#ifdef SYMBIAN
    loopcount = 0;
#endif
    acceptSock = PR_Accept(listenSock, NULL, PR_INTERVAL_NO_TIMEOUT);
    if (NULL == acceptSock) {
        fprintf(stderr, "PR_Accept failed\n");
        exit(1);
    }
    osfd = PR_FileDesc2NativeHandle(acceptSock);
    while ((nbytes = write(osfd, buf, sizeof(buf))) != -1) {
        
#ifdef SYMBIAN
      if (loopcount++>64) break;
#endif
    }
    if ((errno != EAGAIN) && (errno != EWOULDBLOCK)) {
        fprintf(stderr, "write failed\n");
        exit(1);
    }
    printf("calling PR_Send with a zero-length buffer\n");
    fflush(stdout);
    nbytes = PR_Send(acceptSock, buf, 0, 0, PR_INTERVAL_NO_TIMEOUT);
    if (nbytes != 0) {
        fprintf(stderr, "PR_Send should return 0 but returns %d\n", nbytes);
        exit(1);
    }
    if (PR_Close(acceptSock) == PR_FAILURE) {
        fprintf(stderr, "PR_Close failed\n");
        exit(1);
    }
    if (PR_JoinThread(clientThread) == PR_FAILURE) {
        fprintf(stderr, "PR_JoinThread failed\n");
        exit(1);
    }

    if (PR_Close(listenSock) == PR_FAILURE) {
        fprintf(stderr, "PR_Close failed\n");
        exit(1);
    }
    printf("PASS\n");
    return 0;
}

#endif 
