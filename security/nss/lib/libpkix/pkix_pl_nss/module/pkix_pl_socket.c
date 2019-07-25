































































#ifndef BUILD_OPT
#define PKIX_SOCKETTRACE 1
#endif

#ifdef PKIX_SOCKETDEBUG
#define PKIX_SOCKETTRACE 1
#endif

#include "pkix_pl_socket.h"



#ifdef PKIX_SOCKETTRACE
static PKIX_Boolean socketTraceFlag = PKIX_FALSE;
















static void pkix_pl_socket_timestamp() {
        PRInt64 prTime;
        prTime = PR_Now();
        printf("%lld:\n", prTime);
}















static void pkix_pl_socket_hexDigit(char byteVal) {
        int n = 0;
        char cHi = '\0';
        char cLow = '\0';
        n = ((byteVal >> 4) & 0xf);
        if (n > 9) {
                cHi = (char) ((n - 10) + 'A');
        } else {
                cHi = (char) (n + '0');
        }
        n = byteVal & 0xf;
        if (n > 9) {
                cLow = (char) ((n - 10) + 'A');
        } else {
                cLow = (char) (n + '0');
        }
        (void) printf("%c%c", cHi, cLow);
}

















static void pkix_pl_socket_linePrefix(PKIX_UInt32 addr) {
        pkix_pl_socket_hexDigit((char)((addr >> 8) & 0xff));
        pkix_pl_socket_hexDigit((char)(addr & 0xff));
        (void) printf(": ");
}



















static void pkix_pl_socket_traceLine(char *ptr) {
        PKIX_UInt32 i = 0;
        pkix_pl_socket_linePrefix((PKIX_UInt32)ptr);
        for (i = 0; i < 16; i++) {
                printf(" ");
                pkix_pl_socket_hexDigit(ptr[i]);
                if (i == 7) {
                        printf("  ");
                }
        }
        printf("  ");
        for (i = 0; i < 16; i++) {
                if ((ptr[i] < ' ') || (ptr[i] > '}')) {
                        printf(".");
                } else {
                        printf("%c", ptr[i]);
                }
        }
        printf("\n");
}























static void pkix_pl_socket_tracePartialLine(char *ptr, PKIX_UInt32 nBytes) {
        PKIX_UInt32 i = 0;
        if (nBytes > 0) {
                pkix_pl_socket_linePrefix((PKIX_UInt32)ptr);
        }
        for (i = 0; i < nBytes; i++) {
                printf(" ");
                pkix_pl_socket_hexDigit(ptr[i]);
                if (i == 7) {
                        printf("  ");
                }
        }
        for (i = nBytes; i < 16; i++) {
                printf("   ");
                if (i == 7) {
                        printf("  ");
                }
        }
        printf("  ");
        for (i = 0; i < nBytes; i++) {
                if ((ptr[i] < ' ') || (ptr[i] > '}')) {
                        printf(".");
                } else {
                        printf("%c", ptr[i]);
                }
        }
        printf("\n");
}






















void pkix_pl_socket_tracebuff(void *buf, PKIX_UInt32 nBytes) {
        PKIX_UInt32 bytesRemaining = nBytes;
        PKIX_UInt32 offset = 0;
        char *bufptr = (char *)buf;

        if (socketTraceFlag == PKIX_FALSE) return;

        pkix_pl_socket_timestamp();
        


        if (nBytes == 0) {
                pkix_pl_socket_linePrefix((PKIX_UInt32)buf);
                printf("\n");
        } else {
                while (bytesRemaining >= 16) {
                        pkix_pl_socket_traceLine(&bufptr[offset]);
                        bytesRemaining -= 16;
                        offset += 16;
                }
                pkix_pl_socket_tracePartialLine
                        (&bufptr[offset], bytesRemaining);
        }
}

#endif



















static PKIX_Error *
pkix_pl_Socket_SetNonBlocking(
        PRFileDesc *fileDesc,
        void *plContext)
{
        PRStatus rv = PR_FAILURE;
        PRSocketOptionData sockOptionData;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_SetNonBlocking");
        PKIX_NULLCHECK_ONE(fileDesc);

        sockOptionData.option = PR_SockOpt_Nonblocking;
        sockOptionData.value.non_blocking = PR_TRUE;

        PKIX_PL_NSSCALLRV(SOCKET, rv, fileDesc->methods->setsocketoption,
                (fileDesc, &sockOptionData));

        if (rv != PR_SUCCESS) {
                PKIX_ERROR(PKIX_UNABLETOSETSOCKETTONONBLOCKING);
        }
cleanup:

        PKIX_RETURN(SOCKET);
}





















static PKIX_Error *
pkix_pl_Socket_CreateClient(
        PKIX_PL_Socket *socket,
        void *plContext)
{
#ifdef PKIX_SOCKETDEBUG
        PRErrorCode errorcode = 0;
#endif
        PRFileDesc *mySock = NULL;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_CreateClient");
        PKIX_NULLCHECK_ONE(socket);

        PKIX_PL_NSSCALLRV(SOCKET, mySock, PR_NewTCPSocket, ());
        if (!mySock) {
#ifdef PKIX_SOCKETDEBUG
                errorcode = PR_GetError();
                printf
                        ("pkix_pl_Socket_CreateClient: %s\n",
                        PR_ErrorToString(errorcode, PR_LANGUAGE_EN));
#endif
                PKIX_ERROR(PKIX_PRNEWTCPSOCKETFAILED);
        }

#ifdef PKIX_SOCKETDEBUG
        printf("Created socket, PRFileDesc @  %#X\n", mySock);
#endif

        socket->clientSock = mySock;
        socket->status = SOCKET_UNCONNECTED;
        if (socket->timeout == 0) {
                PKIX_CHECK(pkix_pl_Socket_SetNonBlocking(mySock, plContext),
                        PKIX_SOCKETSETNONBLOCKINGFAILED);
        }

cleanup:

        PKIX_RETURN(SOCKET);
}
























static PKIX_Error *
pkix_pl_Socket_CreateServer(
        PKIX_PL_Socket *socket,
        void *plContext)
{

        PRErrorCode errorcode = 0;

        PRStatus rv = PR_FAILURE;
        PRFileDesc *serverSock = NULL;
        PRSocketOptionData sockOptionData;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_CreateServer");
        PKIX_NULLCHECK_ONE(socket);

        PKIX_PL_NSSCALLRV(SOCKET, serverSock, PR_NewTCPSocket, ());
        if (!serverSock) {
#ifdef PKIX_SOCKETDEBUG
                errorcode = PR_GetError();
                printf
                        ("pkix_pl_Socket_CreateServer: %s\n",
                        PR_ErrorToString(errorcode, PR_LANGUAGE_EN));
#endif
                PKIX_ERROR(PKIX_PRNEWTCPSOCKETFAILED);
        }

        socket->serverSock = serverSock;

#ifdef PKIX_SOCKETDEBUG
        printf("Created socket, PRFileDesc @  %#X\n", serverSock);
#endif

        if (socket->timeout == 0) {
                PKIX_CHECK(pkix_pl_Socket_SetNonBlocking(serverSock, plContext),
                        PKIX_SOCKETSETNONBLOCKINGFAILED);
        }

        sockOptionData.option = PR_SockOpt_Reuseaddr;
        sockOptionData.value.reuse_addr = PR_TRUE;

        PKIX_PL_NSSCALLRV(SOCKET, rv, serverSock->methods->setsocketoption,
                (serverSock, &sockOptionData));

        if (rv != PR_SUCCESS) {
                PKIX_ERROR(PKIX_UNABLETOSETSOCKETTONONBLOCKING);
        }

        PKIX_PL_NSSCALLRV(SOCKET, rv, PR_Bind, (serverSock, socket->netAddr));

        if (rv == PR_FAILURE) {

                errorcode = PR_GetError();
                printf
                        ("pkix_pl_Socket_CreateServer: %s\n",
                        PR_ErrorToString(errorcode, PR_LANGUAGE_EN));

                PKIX_ERROR(PKIX_PRBINDFAILED);
        }

#ifdef PKIX_SOCKETDEBUG
        printf("Successful bind!\n");
#endif

        socket->status = SOCKET_BOUND;

cleanup:

        PKIX_RETURN(SOCKET);
}





















static PKIX_Error *
pkix_pl_Socket_Connect(
        PKIX_PL_Socket *socket,
        PRErrorCode *pStatus,
        void *plContext)
{
        PRStatus rv = PR_FAILURE;
        PRErrorCode errorcode = 0;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_Connect");
        PKIX_NULLCHECK_TWO(socket, socket->clientSock);

        PKIX_PL_NSSCALLRV(SOCKET, rv, PR_Connect,
                (socket->clientSock, socket->netAddr, socket->timeout));

        if (rv == PR_FAILURE) {
                errorcode = PR_GetError();
                *pStatus = errorcode;
                if (errorcode == PR_IN_PROGRESS_ERROR) {
                        socket->status = SOCKET_CONNECTPENDING;
                        goto cleanup;
                } else {
#ifdef PKIX_SOCKETDEBUG
                        printf
                                ("pkix_pl_Socket_Connect: %s\n",
                                PR_ErrorToString(errorcode, PR_LANGUAGE_EN));
#endif
                        PKIX_ERROR(PKIX_PRCONNECTFAILED);
                }
        }

#ifdef PKIX_SOCKETDEBUG
        printf("Successful connect!\n");
#endif

        *pStatus = 0;
        socket->status = SOCKET_CONNECTED;

cleanup:

        PKIX_RETURN(SOCKET);
}






















static PKIX_Error *
pkix_pl_Socket_ConnectContinue(
        PKIX_PL_Socket *socket,
        PRErrorCode *pStatus,
        void *plContext)
{
        PRStatus rv = PR_FAILURE;
        PRErrorCode errorcode = 0;
        PRPollDesc pollDesc;
        PRInt32 numEvents = 0;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_ConnectContinue");
        PKIX_NULLCHECK_TWO(socket, socket->clientSock);

        pollDesc.fd = socket->clientSock;
        pollDesc.in_flags = PR_POLL_WRITE | PR_POLL_EXCEPT;
        pollDesc.out_flags = 0;
        PKIX_PL_NSSCALLRV(SOCKET, numEvents, PR_Poll, (&pollDesc, 1, 0));
        if (numEvents < 0) {
                PKIX_ERROR(PKIX_PRPOLLFAILED);
        }

        if (numEvents == 0) {
                *pStatus = PR_IN_PROGRESS_ERROR;
                goto cleanup;
        }

        PKIX_PL_NSSCALLRV(SOCKET, rv, PR_ConnectContinue,
                (socket->clientSock, pollDesc.out_flags));

        




        if ((rv == PR_SUCCESS) && (pollDesc.out_flags == PR_POLL_ERR)) {
                *pStatus = PR_IN_PROGRESS_ERROR;
                goto cleanup;
        }

        if (rv == PR_FAILURE) {
                errorcode = PR_GetError();
                *pStatus = errorcode;
                if (errorcode == PR_IN_PROGRESS_ERROR) {
                        goto cleanup;
                } else {
#ifdef PKIX_SOCKETDEBUG
                        printf
                                ("pkix_pl_Socket_ConnectContinue: %s\n",
                                PR_ErrorToString(errorcode, PR_LANGUAGE_EN));
#endif
                        PKIX_ERROR(PKIX_PRCONNECTCONTINUEFAILED);
                }
        }

#ifdef PKIX_SOCKETDEBUG
        printf("Successful connect!\n");
#endif

        *pStatus = 0;
        socket->status = SOCKET_CONNECTED;

cleanup:

        PKIX_RETURN(SOCKET);
}





static PKIX_Error *
pkix_pl_Socket_Destroy(
        PKIX_PL_Object *object,
        void *plContext)
{
        PKIX_PL_Socket *socket = NULL;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_Destroy");
        PKIX_NULLCHECK_ONE(object);

        PKIX_CHECK(pkix_CheckType
                    (object, PKIX_SOCKET_TYPE, plContext),
                    PKIX_OBJECTNOTANSOCKET);

        socket = (PKIX_PL_Socket *)object;

        if (socket->isServer) {
                if (socket->serverSock) {
                        PR_Close(socket->serverSock);
                }
        } else {
                if (socket->clientSock) {
                        PR_Close(socket->clientSock);
                }
        }

cleanup:

        PKIX_RETURN(SOCKET);
}





static PKIX_Error *
pkix_pl_Socket_Hashcode(
        PKIX_PL_Object *object,
        PKIX_UInt32 *pHashcode,
        void *plContext)
{
        PKIX_PL_Socket *socket = NULL;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_Hashcode");
        PKIX_NULLCHECK_TWO(object, pHashcode);

        PKIX_CHECK(pkix_CheckType(object, PKIX_SOCKET_TYPE, plContext),
                PKIX_OBJECTNOTSOCKET);

        socket = (PKIX_PL_Socket *)object;

        *pHashcode = (((socket->timeout << 3) +
                 (socket->netAddr->inet.family << 3)) +
                 (*((PKIX_UInt32 *)&(socket->netAddr->inet.ip)))) +
                 socket->netAddr->inet.port;

cleanup:

        PKIX_RETURN(SOCKET);
}





static PKIX_Error *
pkix_pl_Socket_Equals(
        PKIX_PL_Object *firstObject,
        PKIX_PL_Object *secondObject,
        PKIX_Int32 *pResult,
        void *plContext)
{
        PKIX_PL_Socket *firstSocket = NULL;
        PKIX_PL_Socket *secondSocket = NULL;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_Equals");
        PKIX_NULLCHECK_THREE(firstObject, secondObject, pResult);

        *pResult = PKIX_FALSE;

        PKIX_CHECK(pkix_CheckTypes
                (firstObject, secondObject, PKIX_SOCKET_TYPE, plContext),
                PKIX_OBJECTNOTSOCKET);

        firstSocket = (PKIX_PL_Socket *)firstObject;
        secondSocket = (PKIX_PL_Socket *)secondObject;

        if (firstSocket->timeout != secondSocket->timeout) {
                goto cleanup;
        }

        if (firstSocket->netAddr == secondSocket->netAddr) {
                *pResult = PKIX_TRUE;
                goto cleanup;
        }

        if ((firstSocket->netAddr->inet.family !=
                secondSocket->netAddr->inet.family) ||
            (*((PKIX_UInt32 *)&(firstSocket->netAddr->inet.ip)) !=
                *((PKIX_UInt32 *)&(secondSocket->netAddr->inet.ip))) ||
            (firstSocket->netAddr->inet.port !=
                secondSocket->netAddr->inet.port)) {

                goto cleanup;

        }

        *pResult = PKIX_TRUE;

cleanup:

        PKIX_RETURN(SOCKET);
}















PKIX_Error *
pkix_pl_Socket_RegisterSelf(void *plContext)
{
        extern pkix_ClassTable_Entry systemClasses[PKIX_NUMTYPES];
        pkix_ClassTable_Entry entry;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_RegisterSelf");

        entry.description = "Socket";
        entry.objCounter = 0;
        entry.typeObjectSize = sizeof(PKIX_PL_Socket);
        entry.destructor = pkix_pl_Socket_Destroy;
        entry.equalsFunction = pkix_pl_Socket_Equals;
        entry.hashcodeFunction = pkix_pl_Socket_Hashcode;
        entry.toStringFunction = NULL;
        entry.comparator = NULL;
        entry.duplicateFunction = NULL;

        systemClasses[PKIX_SOCKET_TYPE] = entry;

#ifdef PKIX_SOCKETTRACE
        {
                char *val = NULL;
                val = PR_GetEnv("SOCKETTRACE");
                
                if ((val != NULL) && (*val != '\0')) {
                        socketTraceFlag =
                                ((*val == '1')?PKIX_TRUE:PKIX_FALSE);
                }
        }
#endif

        PKIX_RETURN(SOCKET);
}























static PKIX_Error *
pkix_pl_Socket_Listen(
        PKIX_PL_Socket *socket,
        PKIX_UInt32 backlog,
        void *plContext)
{
#ifdef PKIX_SOCKETDEBUG
        PRErrorCode errorcode = 0;
#endif
        PRStatus rv = PR_FAILURE;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_Listen");
        PKIX_NULLCHECK_TWO(socket, socket->serverSock);

        PKIX_PL_NSSCALLRV(SOCKET, rv, PR_Listen,
                (socket->serverSock, (PRIntn)backlog));

        if (rv == PR_FAILURE) {
#ifdef PKIX_SOCKETDEBUG
                errorcode = PR_GetError();
                printf
                        ("pkix_pl_Socket_Listen: %s\n",
                        PR_ErrorToString(errorcode, PR_LANGUAGE_EN));
#endif
                PKIX_ERROR(PKIX_PRLISTENFAILED);
        }

#ifdef PKIX_SOCKETDEBUG
        printf("Successful listen!\n");
#endif

        socket->status = SOCKET_LISTENING;
cleanup:

        PKIX_RETURN(SOCKET);
}


















static PKIX_Error *
pkix_pl_Socket_Shutdown(
        PKIX_PL_Socket *socket,
        void *plContext)
{
#ifdef PKIX_SOCKETDEBUG
        PRErrorCode errorcode = 0;
#endif
        PRStatus rv = PR_FAILURE;
        PRFileDesc *fileDesc = NULL;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_Shutdown");
        PKIX_NULLCHECK_ONE(socket);

        fileDesc =
                (socket->isServer)?(socket->serverSock):(socket->clientSock);

        PKIX_PL_NSSCALLRV(SOCKET, rv, PR_Shutdown,
                (fileDesc, PR_SHUTDOWN_BOTH));

        if (rv == PR_FAILURE) {
#ifdef PKIX_SOCKETDEBUG
                errorcode = PR_GetError();
                printf
                        ("pkix_pl_Socket_Shutdown: %s\n",
                        PR_ErrorToString(errorcode, PR_LANGUAGE_EN));
#endif
                PKIX_ERROR(PKIX_PRSHUTDOWNFAILED);
        }
        socket->status = SOCKET_SHUTDOWN;

cleanup:

        PKIX_RETURN(SOCKET);
}































static PKIX_Error *
pkix_pl_Socket_Send(
        PKIX_PL_Socket *sendSock,
        void *buf,
        PKIX_UInt32 bytesToWrite,
        PKIX_Int32 *pBytesWritten,
        void *plContext)
{
        PRInt32 bytesWritten = 0;
        PRErrorCode errorcode = 0;
        PRFileDesc *fd = NULL;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_Send");
        PKIX_NULLCHECK_TWO(buf, pBytesWritten);

        fd = sendSock->clientSock;

        PKIX_PL_NSSCALLRV(SOCKET, bytesWritten, PR_Send,
                (fd, buf, (PRInt32)bytesToWrite, 0, sendSock->timeout));

        if (bytesWritten >= 0) {
                if (sendSock->status == SOCKET_SENDRCVPENDING) {
                        sendSock->status = SOCKET_RCVPENDING;
                } else {
                        sendSock->status = SOCKET_CONNECTED;
                }
#ifdef PKIX_SOCKETTRACE
                pkix_pl_socket_tracebuff(buf, bytesWritten);
#endif
        } else {
                errorcode = PR_GetError();
                if (errorcode != PR_WOULD_BLOCK_ERROR) {
#ifdef PKIX_SOCKETDEBUG
                        printf
                                ("pkix_pl_Socket_Send: %s\n",
                                PR_ErrorToString(errorcode, PR_LANGUAGE_EN));
#endif
                        PKIX_ERROR(PKIX_PRSENDFAILED);
                }

                sendSock->writeBuf = buf;
                sendSock->writeBufSize = bytesToWrite;
                if (sendSock->status == SOCKET_RCVPENDING) {
                        sendSock->status = SOCKET_SENDRCVPENDING;
                } else {
                        sendSock->status = SOCKET_SENDPENDING;
                }
        }

        *pBytesWritten = (PKIX_Int32)bytesWritten;

cleanup:

        PKIX_RETURN(SOCKET);
}

































static PKIX_Error *
pkix_pl_Socket_Recv(
        PKIX_PL_Socket *rcvSock,
        void *buf,
        PKIX_UInt32 capacity,
        PKIX_Int32 *pBytesRead,
        void *plContext)
{
        PRErrorCode errorcode = 0;
        PRInt32 bytesRead = 0;
        PRFileDesc *fd = NULL;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_Recv");
        PKIX_NULLCHECK_THREE(rcvSock, buf, pBytesRead);

        fd = rcvSock->clientSock;

        PKIX_PL_NSSCALLRV(SOCKET, bytesRead, PR_Recv,
                (fd, buf, (PRInt32)capacity, 0, rcvSock->timeout));

        if (bytesRead > 0) {
                if (rcvSock->status == SOCKET_SENDRCVPENDING) {
                        rcvSock->status = SOCKET_SENDPENDING;
                } else {
                        rcvSock->status = SOCKET_CONNECTED;
                }
#ifdef PKIX_SOCKETTRACE
                pkix_pl_socket_tracebuff(buf, bytesRead);
#endif
        } else if (bytesRead == 0) {
                PKIX_ERROR(PKIX_PRRECVREPORTSNETWORKCONNECTIONCLOSED);
        } else {
                errorcode = PR_GetError();
                if (errorcode != PR_WOULD_BLOCK_ERROR) {
#ifdef PKIX_SOCKETDEBUG
                        printf
                                ("pkix_pl_Socket_Recv: %s\n",
                                PR_ErrorToString(errorcode, PR_LANGUAGE_EN));
#endif
                        PKIX_ERROR(PKIX_PRRECVFAILED);
                }
                rcvSock->readBuf = buf;
                rcvSock->readBufSize = capacity;
                if (rcvSock->status == SOCKET_SENDPENDING) {
                        rcvSock->status = SOCKET_SENDRCVPENDING;
                } else {
                        rcvSock->status = SOCKET_RCVPENDING;
                }

        }

        *pBytesRead = (PKIX_Int32)bytesRead;

cleanup:

        PKIX_RETURN(SOCKET);
}





























static PKIX_Error *
pkix_pl_Socket_Poll(
        PKIX_PL_Socket *sock,
        PKIX_Int32 *pBytesWritten,
        PKIX_Int32 *pBytesRead,
        void *plContext)
{
        PRPollDesc pollDesc;
        PRInt32 numEvents = 0;
        PKIX_Int32 bytesRead = 0;
        PKIX_Int32 bytesWritten = 0;
        PRErrorCode errorcode = 0;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_Poll");
        PKIX_NULLCHECK_ONE(sock);

        pollDesc.fd = sock->clientSock;
        pollDesc.in_flags = 0;
        pollDesc.out_flags = 0;

        if ((pBytesWritten) &&
            ((sock->status == SOCKET_SENDPENDING) ||
            (sock->status = SOCKET_SENDRCVPENDING))) {
                pollDesc.in_flags = PR_POLL_WRITE;
        }

        if ((pBytesRead) &&
            ((sock->status = SOCKET_RCVPENDING) ||
            (sock->status = SOCKET_SENDRCVPENDING))) {
                pollDesc.in_flags |= PR_POLL_READ;
        }

        PKIX_PL_NSSCALLRV(SOCKET, numEvents, PR_Poll, (&pollDesc, 1, 0));

        if (numEvents < 0) {
                PKIX_ERROR(PKIX_PRPOLLFAILED);
        } else if (numEvents > 0) {
                if (pollDesc.out_flags & PR_POLL_WRITE) {
                        PKIX_CHECK(pkix_pl_Socket_Send
                                (sock,
                                sock->writeBuf,
                                sock->writeBufSize,
                                &bytesWritten,
                                plContext),
                                PKIX_SOCKETSENDFAILED);
                        *pBytesWritten = (PKIX_Int32)bytesWritten;
                        if (bytesWritten >= 0) {
                                sock->writeBuf = NULL;
                                sock->writeBufSize = 0;
                        }
                }

                if (pollDesc.out_flags & PR_POLL_READ) {
                        PKIX_CHECK(pkix_pl_Socket_Recv
                                (sock,
                                sock->readBuf,
                                sock->readBufSize,
                                &bytesRead,
                                plContext),
                                PKIX_SOCKETRECVFAILED);
                        *pBytesRead = (PKIX_Int32)bytesRead;
                        if (bytesRead >= 0) {
                                sock->readBuf = NULL;
                                sock->readBufSize = 0;
                        }
                }
        } else if (numEvents == 0) {
                errorcode = PR_GetError();
                if (errorcode != PR_WOULD_BLOCK_ERROR) {
#ifdef PKIX_SOCKETDEBUG
                        printf
                                ("pkix_pl_Socket_Poll: %s\n",
                                PR_ErrorToString(errorcode, PR_LANGUAGE_EN));
#endif
                        PKIX_ERROR(PKIX_PRPOLLFAILED);
                }
                if (pBytesWritten) {
                        *pBytesWritten = 0;
                }
                if (pBytesRead) {
                        *pBytesRead = 0;
                }
        }

cleanup:

        PKIX_RETURN(SOCKET);
}




























static PKIX_Error *
pkix_pl_Socket_Accept(
        PKIX_PL_Socket *serverSocket,
        PKIX_PL_Socket **pRendezvousSocket,
        void *plContext)
{
        PRErrorCode errorcode = 0;
        PRFileDesc *rendezvousSock = NULL;
        PRNetAddr *clientAddr = NULL;
        PKIX_PL_Socket *newSocket = NULL;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_Accept");
        PKIX_NULLCHECK_TWO(serverSocket, pRendezvousSocket);

        PKIX_PL_NSSCALLRV(SOCKET, rendezvousSock, PR_Accept,
                (serverSocket->serverSock, clientAddr, serverSocket->timeout));

        if (!rendezvousSock) {
                errorcode = PR_GetError();
                if (errorcode != PR_WOULD_BLOCK_ERROR) {
#ifdef PKIX_SOCKETDEBUG
                        printf
                                ("pkix_pl_Socket_Accept: %s\n",
                                PR_ErrorToString(errorcode, PR_LANGUAGE_EN));
#endif
                        PKIX_ERROR(PKIX_PRACCEPTFAILED);
                }
                serverSocket->status = SOCKET_ACCEPTPENDING;
                *pRendezvousSocket = NULL;
                goto cleanup;

        }

#ifdef PKIX_SOCKETDEBUG
        printf("Successful accept!\n");
#endif

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_SOCKET_TYPE,
                    sizeof (PKIX_PL_Socket),
                    (PKIX_PL_Object **)&newSocket,
                    plContext),
                    PKIX_COULDNOTCREATESOCKETOBJECT);

        newSocket->isServer = PKIX_FALSE;
        newSocket->timeout = serverSocket->timeout;
        newSocket->clientSock = rendezvousSock;
        newSocket->serverSock = NULL;
        newSocket->netAddr = NULL;
        newSocket->status = SOCKET_CONNECTED;
        newSocket->callbackList.shutdownCallback = pkix_pl_Socket_Shutdown;
        newSocket->callbackList.listenCallback = pkix_pl_Socket_Listen;
        newSocket->callbackList.acceptCallback = pkix_pl_Socket_Accept;
        newSocket->callbackList.connectcontinueCallback =
                pkix_pl_Socket_ConnectContinue;
        newSocket->callbackList.sendCallback = pkix_pl_Socket_Send;
        newSocket->callbackList.recvCallback = pkix_pl_Socket_Recv;
        newSocket->callbackList.pollCallback = pkix_pl_Socket_Poll;

        if (serverSocket->timeout == 0) {
                PKIX_CHECK(pkix_pl_Socket_SetNonBlocking
                        (rendezvousSock, plContext),
                        PKIX_SOCKETSETNONBLOCKINGFAILED);
        }

        *pRendezvousSocket = newSocket;

cleanup:

        PKIX_RETURN(SOCKET);
}

































PKIX_Error *
pkix_pl_Socket_Create(
        PKIX_Boolean isServer,
        PRIntervalTime timeout,
        PRNetAddr *netAddr,
        PRErrorCode *status,
        PKIX_PL_Socket **pSocket,
        void *plContext)
{
        PKIX_PL_Socket *socket = NULL;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_Create");
        PKIX_NULLCHECK_ONE(pSocket);

        PKIX_CHECK(PKIX_PL_Object_Alloc
                    (PKIX_SOCKET_TYPE,
                    sizeof (PKIX_PL_Socket),
                    (PKIX_PL_Object **)&socket,
                    plContext),
                    PKIX_COULDNOTCREATESOCKETOBJECT);

        socket->isServer = isServer;
        socket->timeout = timeout;
        socket->clientSock = NULL;
        socket->serverSock = NULL;
        socket->netAddr = netAddr;

        socket->callbackList.listenCallback = pkix_pl_Socket_Listen;
        socket->callbackList.acceptCallback = pkix_pl_Socket_Accept;
        socket->callbackList.connectcontinueCallback =
                 pkix_pl_Socket_ConnectContinue;
        socket->callbackList.sendCallback = pkix_pl_Socket_Send;
        socket->callbackList.recvCallback = pkix_pl_Socket_Recv;
        socket->callbackList.pollCallback = pkix_pl_Socket_Poll;
        socket->callbackList.shutdownCallback = pkix_pl_Socket_Shutdown;

        if (isServer) {
                PKIX_CHECK(pkix_pl_Socket_CreateServer(socket, plContext),
                        PKIX_SOCKETCREATESERVERFAILED);
                *status = 0;
        } else {
                socket->timeout = timeout;
                PKIX_CHECK(pkix_pl_Socket_CreateClient(socket, plContext),
                        PKIX_SOCKETCREATECLIENTFAILED);
                PKIX_CHECK(pkix_pl_Socket_Connect(socket, status, plContext),
                        PKIX_SOCKETCONNECTFAILED);
        }

        *pSocket = socket;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(socket);
        }

        PKIX_RETURN(SOCKET);
}







































PKIX_Error *
pkix_pl_Socket_CreateByName(
        PKIX_Boolean isServer,
        PRIntervalTime timeout,
        char *serverName,
        PRErrorCode *pStatus,
        PKIX_PL_Socket **pSocket,
        void *plContext)
{
        PRNetAddr netAddr;
        PKIX_PL_Socket *socket = NULL;
        char *sepPtr = NULL;
        PRHostEnt hostent;
        PRIntn hostenum;
        PRStatus prstatus = PR_FAILURE;
        char buf[PR_NETDB_BUF_SIZE];
        PRUint16 portNum = 0;
        char *localCopyName = NULL;

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_CreateByName");
        PKIX_NULLCHECK_TWO(serverName, pSocket);

        localCopyName = PL_strdup(serverName);

        sepPtr = strchr(localCopyName, ':');
        
        if (sepPtr) {
                *sepPtr++ = '\0';
                 portNum = (PRUint16)atoi(sepPtr);
        } else {
                 portNum = (PRUint16)LDAP_PORT;
        }

        prstatus = PR_GetHostByName(localCopyName, buf, sizeof(buf), &hostent);

        if ((prstatus != PR_SUCCESS) || (hostent.h_length != 4)) {
                



                sepPtr = strchr(localCopyName, '.');
                if (sepPtr) {
                        *sepPtr++ = '\0';
                }
                prstatus = PR_GetHostByName
                        (localCopyName, buf, sizeof(buf), &hostent);

                if ((prstatus != PR_SUCCESS) || (hostent.h_length != 4)) {
                        PKIX_ERROR
                                (PKIX_PRGETHOSTBYNAMEREJECTSHOSTNAMEARGUMENT);
                }
        }

        netAddr.inet.family = PR_AF_INET;
        netAddr.inet.port = PR_htons(portNum);

        if (isServer) {

                netAddr.inet.ip = PR_INADDR_ANY;

        } else {

                hostenum = PR_EnumerateHostEnt(0, &hostent, portNum, &netAddr);
                if (hostenum == -1) {
                        PKIX_ERROR(PKIX_PRENUMERATEHOSTENTFAILED);
                }
        }

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_SOCKET_TYPE,
                sizeof (PKIX_PL_Socket),
                (PKIX_PL_Object **)&socket,
                plContext),
                PKIX_COULDNOTCREATESOCKETOBJECT);

        socket->isServer = isServer;
        socket->timeout = timeout;
        socket->clientSock = NULL;
        socket->serverSock = NULL;
        socket->netAddr = &netAddr;

        socket->callbackList.listenCallback = pkix_pl_Socket_Listen;
        socket->callbackList.acceptCallback = pkix_pl_Socket_Accept;
        socket->callbackList.connectcontinueCallback =
                 pkix_pl_Socket_ConnectContinue;
        socket->callbackList.sendCallback = pkix_pl_Socket_Send;
        socket->callbackList.recvCallback = pkix_pl_Socket_Recv;
        socket->callbackList.pollCallback = pkix_pl_Socket_Poll;
        socket->callbackList.shutdownCallback = pkix_pl_Socket_Shutdown;

        if (isServer) {
                PKIX_CHECK(pkix_pl_Socket_CreateServer(socket, plContext),
                        PKIX_SOCKETCREATESERVERFAILED);
                *pStatus = 0;
        } else {
                PKIX_CHECK(pkix_pl_Socket_CreateClient(socket, plContext),
                        PKIX_SOCKETCREATECLIENTFAILED);
                PKIX_CHECK(pkix_pl_Socket_Connect(socket, pStatus, plContext),
                        PKIX_SOCKETCONNECTFAILED);
        }

        *pSocket = socket;

cleanup:
        PL_strfree(localCopyName);

        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(socket);
        }

        PKIX_RETURN(SOCKET);
}








































PKIX_Error *
pkix_pl_Socket_CreateByHostAndPort(
        PKIX_Boolean isServer,
        PRIntervalTime timeout,
        char *hostname,
        PRUint16 portnum,
        PRErrorCode *pStatus,
        PKIX_PL_Socket **pSocket,
        void *plContext)
{
        PRNetAddr netAddr;
        PKIX_PL_Socket *socket = NULL;
        char *sepPtr = NULL;
        PRHostEnt hostent;
        PRIntn hostenum;
        PRStatus prstatus = PR_FAILURE;
        char buf[PR_NETDB_BUF_SIZE];

        PKIX_ENTER(SOCKET, "pkix_pl_Socket_CreateByHostAndPort");
        PKIX_NULLCHECK_THREE(hostname, pStatus, pSocket);


        prstatus = PR_GetHostByName(hostname, buf, sizeof(buf), &hostent);

        if ((prstatus != PR_SUCCESS) || (hostent.h_length != 4)) {
                



                sepPtr = strchr(hostname, '.');
                if (sepPtr) {
                        *sepPtr++ = '\0';
                }
                prstatus = PR_GetHostByName(hostname, buf, sizeof(buf), &hostent);

                if ((prstatus != PR_SUCCESS) || (hostent.h_length != 4)) {
                        PKIX_ERROR
                                (PKIX_PRGETHOSTBYNAMEREJECTSHOSTNAMEARGUMENT);
                }
        }

        netAddr.inet.family = PR_AF_INET;
        netAddr.inet.port = PR_htons(portnum);

        if (isServer) {

                netAddr.inet.ip = PR_INADDR_ANY;

        } else {

                hostenum = PR_EnumerateHostEnt(0, &hostent, portnum, &netAddr);
                if (hostenum == -1) {
                        PKIX_ERROR(PKIX_PRENUMERATEHOSTENTFAILED);
                }
        }

        PKIX_CHECK(PKIX_PL_Object_Alloc
                (PKIX_SOCKET_TYPE,
                sizeof (PKIX_PL_Socket),
                (PKIX_PL_Object **)&socket,
                plContext),
                PKIX_COULDNOTCREATESOCKETOBJECT);

        socket->isServer = isServer;
        socket->timeout = timeout;
        socket->clientSock = NULL;
        socket->serverSock = NULL;
        socket->netAddr = &netAddr;

        socket->callbackList.listenCallback = pkix_pl_Socket_Listen;
        socket->callbackList.acceptCallback = pkix_pl_Socket_Accept;
        socket->callbackList.connectcontinueCallback =
                 pkix_pl_Socket_ConnectContinue;
        socket->callbackList.sendCallback = pkix_pl_Socket_Send;
        socket->callbackList.recvCallback = pkix_pl_Socket_Recv;
        socket->callbackList.pollCallback = pkix_pl_Socket_Poll;
        socket->callbackList.shutdownCallback = pkix_pl_Socket_Shutdown;

        if (isServer) {
                PKIX_CHECK(pkix_pl_Socket_CreateServer(socket, plContext),
                        PKIX_SOCKETCREATESERVERFAILED);
                *pStatus = 0;
        } else {
                PKIX_CHECK(pkix_pl_Socket_CreateClient(socket, plContext),
                        PKIX_SOCKETCREATECLIENTFAILED);
                PKIX_CHECK(pkix_pl_Socket_Connect(socket, pStatus, plContext),
                        PKIX_SOCKETCONNECTFAILED);
        }

        *pSocket = socket;

cleanup:
        if (PKIX_ERROR_RECEIVED) {
                PKIX_DECREF(socket);
        }

        PKIX_RETURN(SOCKET);
}




PKIX_Error *
pkix_pl_Socket_GetCallbackList(
        PKIX_PL_Socket *socket,
        PKIX_PL_Socket_Callback **pCallbackList,
        void *plContext)
{
        PKIX_ENTER(SOCKET, "pkix_pl_Socket_GetCallbackList");
        PKIX_NULLCHECK_TWO(socket, pCallbackList);

        *pCallbackList = &(socket->callbackList);

        PKIX_RETURN(SOCKET);
}




PKIX_Error *
pkix_pl_Socket_GetPRFileDesc(
        PKIX_PL_Socket *socket,
        PRFileDesc **pDesc,
        void *plContext)
{
        PKIX_ENTER(SOCKET, "pkix_pl_Socket_GetPRFileDesc");
        PKIX_NULLCHECK_TWO(socket, pDesc);

        *pDesc = socket->clientSock;

        PKIX_RETURN(SOCKET);
}
