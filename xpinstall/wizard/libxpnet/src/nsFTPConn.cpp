






































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <sys/stat.h>

#if defined(__unix) || defined(__unix__) || defined(_AIX)
#include <sys/param.h>
#elif defined(_WINDOWS)
#include <windows.h>
#define MAXPATHLEN MAX_PATH
#elif defined(macintosh)
#define MAXPATHLEN 512
#elif defined(__OS2__)
#include <os2.h>
#define MAXPATHLEN CCHMAXPATH
#endif

#include "nsSocket.h"
#include "nsFTPConn.h"

const int kCntlPort = 21;
const int kDataPort = 20;
const int kCmdBufSize = 64 + MAXPATHLEN;
const int kRespBufSize = 1024;
const int kKilobyte = 1024;
const int kUsecsPerSec = 1000000;
const int kDlBufSize = 1024;
const int kMaxBailOnTimeOut = 50;

nsFTPConn::nsFTPConn(char *aHost, int (*aEventPumpCB)(void)) :
    mEventPumpCB(aEventPumpCB),
    mHost(aHost),
    mState(CLOSED),
    mPassive(FALSE),
    mCntlSock(NULL),
    mDataSock(NULL)
{
}

nsFTPConn::nsFTPConn(char *aHost) :
    mEventPumpCB(NULL),
    mHost(aHost),
    mState(CLOSED),
    mPassive(FALSE),
    mCntlSock(NULL),
    mDataSock(NULL)
{
}

nsFTPConn::~nsFTPConn()
{
    
}

int
nsFTPConn::Open()
{
    int err = OK;
    char cmd[kCmdBufSize], resp[kRespBufSize];

    if (!mHost)
        return E_PARAM;
    if (mState != CLOSED)
        return E_ALREADY_OPEN;

    
    mCntlSock = new nsSocket(mHost, kCntlPort, mEventPumpCB);
    if (!mCntlSock)
        return E_MEM;
    ERR_CHECK(mCntlSock->Open());
    FlushCntlSock(mCntlSock);

    
    err = IssueCmd("USER anonymous\r\n", resp, kRespBufSize, mCntlSock);

    
    ERR_CHECK(IssueCmd("PASS -linux@installer.sbg\r\n", resp, kRespBufSize, mCntlSock));

    mState = OPEN;

    return err;

BAIL:
    if (mCntlSock)
    {
        
        sprintf(cmd, "QUIT\r\n");
        IssueCmd(cmd, resp, kRespBufSize, mCntlSock);
    }
    if (mDataSock)
    {
        mDataSock->Close();
        delete mDataSock;
        mDataSock = NULL;
        FlushCntlSock(mCntlSock);
    }
    if (mCntlSock)
    {
        mCntlSock->Close();
        delete mCntlSock;
        mCntlSock = NULL;
    }
    return err;
}

int
nsFTPConn::Open(char *aHost)
{
    if (!aHost)
        return E_PARAM;

    mHost = aHost;
    return Open();
}

int
nsFTPConn::ResumeOrGet(char *aSrvPath, char *aLoclPath, int aType, 
                       int aOvWrite, FTPGetCB aCBFunc)
{
    struct stat stbuf;
    int err = OK;
    int resPos = 0;

    if (!aLoclPath)
        return E_PARAM;

    
    err = stat(aLoclPath, &stbuf);
    if (err == 0)
        resPos = stbuf.st_size;

    return Get(aSrvPath, aLoclPath, aType, resPos, aOvWrite, aCBFunc);
}

int
nsFTPConn::Get(char *aSrvPath, char *aLoclPath, int aType,
               int aOvWrite, FTPGetCB aCBFunc)
{
    

    return ResumeOrGet(aSrvPath, aLoclPath, aType, aOvWrite, aCBFunc);
}

int
nsFTPConn::Get(char *aSrvPath, char *aLoclPath, int aType, int aResumePos,
               int aOvWrite, FTPGetCB aCBFunc)
{
    struct stat dummy;
    int err = OK, totBytesRd = 0;
    char cmd[kCmdBufSize], resp[kRespBufSize];
    int fileSize = 0;
    FILE *loclfd = NULL;

    if (!aSrvPath || !aLoclPath)
        return E_PARAM;
    if (mState != OPEN || !mCntlSock)
        return E_NOT_OPEN;

    
    err = stat(aLoclPath, &dummy);
    if (err != -1 && aOvWrite == FALSE)
        return E_CANT_OVWRITE;

    mState = GETTING;

    
    ERR_CHECK(DataInit(mHost, kDataPort, &mDataSock));

    
    sprintf(cmd, "TYPE %s\r\n", aType==BINARY ? "I" : "A");
    ERR_CHECK(IssueCmd(cmd, resp, kRespBufSize, mCntlSock));
    
    
    sprintf(cmd, "SIZE %s\r\n", aSrvPath);
    err = IssueCmd(cmd, resp, kRespBufSize, mCntlSock); 
    if (err == OK && (resp[0] == '2'))
        fileSize = atoi(&resp[4]);   

    if (aResumePos > 0)
    {
        
        sprintf(cmd, "REST %d\r\n", aResumePos);
        ERR_CHECK(IssueCmd(cmd, resp, kRespBufSize, mCntlSock));
    }

    
    sprintf(cmd, "RETR %s\r\n", aSrvPath);
    ERR_CHECK(IssueCmd(cmd, resp, kRespBufSize, mCntlSock));

    
    if (!mPassive)
        ERR_CHECK(mDataSock->SrvAccept());

    
    if (aResumePos <= 0)
    {
        if (!(loclfd = fopen(aLoclPath, aType==BINARY ? "w+b" : "w+")))
        {
            err = E_LOCL_INIT;
            goto BAIL;
        }
    }
    else
    {
        if (!(loclfd = fopen(aLoclPath, aType==BINARY ? "r+b" : "r+")) ||
            (fseek(loclfd, aResumePos, SEEK_SET) != 0)) 
        {
            err = E_LOCL_INIT;
            goto BAIL;
        }
    }

    do 
    {
        int respBufSize = kDlBufSize;
        err = mDataSock->Recv((unsigned char *)resp, &respBufSize);
        if (err != nsSocket::E_READ_MORE && 
            err != nsSocket::E_EOF_FOUND &&
            err != nsSocket::OK)
            goto BAIL;
        totBytesRd += respBufSize;
        if (err == nsSocket::E_READ_MORE && aCBFunc)
            if ((err = aCBFunc(totBytesRd, fileSize)) == E_USER_CANCEL)
                goto BAIL;
            
        
        const int wrote = fwrite((void *)resp, 1, respBufSize, loclfd);
        if (wrote != respBufSize)
        {   
            err = E_WRITE;
            goto BAIL;
        }
        if ( mEventPumpCB )
            mEventPumpCB();
    }
    while (err == nsSocket::E_READ_MORE || err == nsSocket::OK);
    if (err == nsSocket::E_EOF_FOUND)
        err = OK;

BAIL:
	if ( err != OK && err != E_USER_CANCEL ) {
		sprintf(cmd, "QUIT\r\n");
		IssueCmd(cmd, resp, kRespBufSize, mCntlSock);
    	FlushCntlSock(mCntlSock);
    }
    
    
    if (loclfd)
        fclose(loclfd);
    if ( err == E_USER_CANCEL ) {
        return err;
    }
    
    if (mDataSock)
    {
        




        int bailOnTimeOut = err != OK ? 1 : kMaxBailOnTimeOut;

        mDataSock->Close();
        delete mDataSock;
        mDataSock = NULL;

        




        FlushCntlSock(mCntlSock, bailOnTimeOut);
    }

    mState = OPEN;
    mPassive = FALSE;

    return err;
}

int 
nsFTPConn::Close()
{
    int err = OK;
    char cmd[kCmdBufSize], resp[kRespBufSize];

    
    if (mCntlSock)
    {
        
        sprintf(cmd, "ABORT\r\n");
        IssueCmd(cmd, resp, kRespBufSize, mCntlSock);
    }
    if (mDataSock)
    {
        mDataSock->Close();
        delete mDataSock;
        mDataSock = NULL;
        FlushCntlSock(mCntlSock);
    }
    if (mCntlSock)
    {
        mCntlSock->Close();
        delete mCntlSock;
        mCntlSock = NULL;
    }

    return err;
}

int
nsFTPConn::FlushCntlSock(nsSocket *aSock, int bailOnTimeOut)
{
    int err = OK;
    char resp[kRespBufSize];
    int bailOnTimeOutCount = 0;

    
    if (!aSock)
        return E_PARAM;

    do
    {
        
        const int timeout = 300000;
        ++bailOnTimeOutCount;
        int respSize = kRespBufSize;
        err = aSock->Recv((unsigned char *)resp, &respSize, timeout);
        if (err != nsSocket::OK && 
            err != nsSocket::E_READ_MORE && 
            err != nsSocket::E_EOF_FOUND)
        {
            if((err == nsSocket::E_TIMEOUT) &&
               (bailOnTimeOutCount < bailOnTimeOut))
                
                err = nsSocket::E_READ_MORE;
            else
                goto BAIL;
        }

        DUMP(resp);
        if ( mEventPumpCB )
            if (mEventPumpCB() == E_USER_CANCEL)
                return E_USER_CANCEL;
    }
    while (err == nsSocket::E_READ_MORE);

    switch (*resp)
    {
        case '2':
            break;
        case '1':
        case '3':
            err = E_CMD_ERR;
            break;
        case '4':
        case '5':
            err = E_CMD_FAIL;
            break;
        default:
            err = E_CMD_UNEXPECTED;
            break;
    }

BAIL:
    return err;
}

int
nsFTPConn::IssueCmd(const char *aCmd, char *aResp, int aRespSize, nsSocket *aSock)
{
    int err = OK;
    int len;

    
    if (!aSock || !aCmd || !aResp || aRespSize <= 0)
        return E_PARAM;

    
    len = strlen(aCmd);
    ERR_CHECK(aSock->Send((unsigned char *)aCmd, &len));
    DUMP(aCmd);

    
    do
    {
        int respSize = aRespSize;
        err = aSock->Recv((unsigned char *)aResp, &respSize);
        if (err != nsSocket::OK && 
            err != nsSocket::E_READ_MORE && 
            err != nsSocket::E_EOF_FOUND)
            goto BAIL;
        DUMP(aResp);
        if ( mEventPumpCB )
            if (mEventPumpCB() == E_USER_CANCEL)
                return E_USER_CANCEL;
    }
    while (err == nsSocket::E_READ_MORE);

    
    if ( (strncmp(aCmd, "APPE", 4) == 0) ||
         (strncmp(aCmd, "LIST", 4) == 0) ||
         (strncmp(aCmd, "NLST", 4) == 0) ||
         (strncmp(aCmd, "REIN", 4) == 0) ||
         (strncmp(aCmd, "RETR", 4) == 0) ||
         (strncmp(aCmd, "STOR", 4) == 0) ||
         (strncmp(aCmd, "STOU", 4) == 0) )
    {
        switch (*aResp)
        {
            case '1':   
            case '2':
                break;
            case '3':
                err = E_CMD_ERR;
                break;
            case '4':
            case '5':
                err = E_CMD_FAIL;
                break;
            default:
                err = E_CMD_UNEXPECTED;
                break;
        }
    }

    
    else if (strncmp(aCmd, "REST", 4) == 0)
    {
        switch (*aResp)
        {
            case '1':
            case '2':
                err = E_CMD_ERR;
                break;
            case '3':
                break;
            case '4':
            case '5':
                err = E_CMD_FAIL;
                break;
            default:
                err = E_CMD_UNEXPECTED;
                break;
        }
    }

    
    else if (strncmp(aCmd, "QUIT", 4) == 0)
    {
        err = OK;
        goto BAIL;
    }

    
    else
    {
        switch (*aResp)
        {
            case '2':
                break;
            case '1':
            case '3':
                err = E_CMD_ERR;
                break;
            case '4':
            case '5':
                err = E_CMD_FAIL;
                break;
            default:
                err = E_CMD_UNEXPECTED;
                break;
        }
    }

BAIL:
    return err;
}

int
nsFTPConn::ParseAddr(char *aBuf, char **aHost, int *aPort)
{
    int err = OK;
    char *c;
    int addr[6];

    
    if (!aBuf || !aHost || !aPort)
        return E_PARAM;

    c = aBuf + strlen("227 "); 
    while (!isdigit((int)(*c)))
    {
        if (*c == '\0')
            return E_INVALID_ADDR;
        c++;
    }

    if (sscanf(c, "%d,%d,%d,%d,%d,%d", 
        &addr[0], &addr[1], &addr[2], &addr[3], &addr[4], &addr[5]) != 6)
        return E_INVALID_ADDR;

    *aHost = (char *)malloc(strlen("XXX.XXX.XXX.XXX") + 1);
    sprintf(*aHost, "%d.%d.%d.%d", addr[0], addr[1], addr[2], addr[3]);

    *aPort = ((addr[4] & 0xFF) << 8) | (addr[5] & 0xFF);

#ifdef DEBUG
    printf("%s %d: PASV response: %d,%d,%d,%d,%d,%d\n", __FILE__, __LINE__,
            addr[0], addr[1], addr[2], addr[3], addr[4], addr[5]);
    printf("%s %d: Host = %s\tPort = %d\n", __FILE__, __LINE__, *aHost, *aPort);
#endif
    
    return err;
}

int
nsFTPConn::DataInit(char *aHost, int aPort, nsSocket **aSock)
{
    int err = OK;
    char cmd[kCmdBufSize], resp[kRespBufSize];
    char *srvHost = NULL; 
    int srvPort = 0;
    char *hostPort = NULL;

    
    if (!aHost || !aSock)
        return E_PARAM;

    
    sprintf(cmd, "PASV\r\n");
    err = IssueCmd(cmd, resp, kRespBufSize, mCntlSock);
    if (err != OK)
    {
        err = OK;
        goto ACTIVE; 
    }
       
    mPassive = TRUE;

    ERR_CHECK(ParseAddr(resp, &srvHost, &srvPort));
    *aSock = new nsSocket(srvHost, srvPort, mEventPumpCB);
    if (!*aSock)
    {
        err = E_MEM;
        goto BAIL;
    }
    
    ERR_CHECK((*aSock)->Open());

    if (srvHost) 
    {
        free(srvHost);
        srvHost = NULL;
    }

    return err;

ACTIVE:

    *aSock = new nsSocket(aHost, aPort, mEventPumpCB);
    if (!*aSock)
    {
        err = E_MEM;
        goto BAIL;
    }

    
    ERR_CHECK((*aSock)->SrvOpen());

    ERR_CHECK((*aSock)->GetHostPortString(&hostPort)); 
    if (!hostPort)
    {
        err = E_MEM;
        goto BAIL;
    }

    sprintf(cmd, "PORT %s\r\n", hostPort);
    ERR_CHECK(IssueCmd(cmd, resp, kRespBufSize, mCntlSock));

BAIL:
    if (mPassive && err != OK)
        mPassive = FALSE;

    if (err != OK && (*aSock))
    {
        delete *aSock;
        *aSock = NULL;
    }

    if (srvHost)
    {
        free(srvHost);
        srvHost = NULL;
    }

    if (hostPort)
    {
        free(hostPort);
        hostPort = NULL;
    }

    return err;
}

#ifdef TEST_NSFTPCONN
static struct timeval init;
int
TestFTPGetCB(int aBytesRd, int aTotal)
{
    struct timeval now;
    float rate;

    gettimeofday(&now, NULL);
    rate = nsSocket::CalcRate(&init, &now, aBytesRd);
    printf("br=%d\ttot=%d\trt=%f\tirt=%d\n",aBytesRd, aTotal, rate, (int)rate);
    return 0;
}

int
main(int argc, char **argv)
{
    int err = nsFTPConn::OK;
    nsFTPConn *conn = 0;
    char *leaf = NULL;
    
    if (argc < 2)
    {
        printf("usage: %s <host> <path/on/server>\n", argv[0]);
        exit(0);
    }

    if ((leaf = strrchr(argv[2], '/'))) leaf++;
    else leaf = argv[2];

    conn = new nsFTPConn(argv[1]);

    printf("Opening connection to %s...\n", argv[1]);
    err = conn->Open();
    if (err != nsFTPConn::OK) { printf("error: %d\n", err); exit(err); }

    printf("Getting binary file %s...\n", argv[2]);
    gettimeofday(&init, NULL);
    err = conn->Get(argv[2], leaf, nsFTPConn::BINARY, TRUE, TestFTPGetCB);
    if (err != nsFTPConn::OK) { printf("error: %d\n", err); exit(err); }

    printf("Closing connection to %s...\n", argv[1]);
    err = conn->Close();
    if (err != nsFTPConn::OK) { printf("error: %d\n", err); exit(err); }

    printf("Test successful!\n");
    exit(err);
}
#endif 
