






































#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <sys/stat.h>
#include "nsHTTPConn.h"
#include "nsSocket.h"

const char kHTTPProto[8] = "http://";
const char kFTPProto[8] = "ftp://";
const int kHTTPPort = 80;
const int kFTPPort = 21;
const int kRespBufSize = 1024;
const int kReqBufSize = 4096;
const int kHdrBufSize = 4096;
const char kCRLF[3] = "\r\n";
const char kHdrBodyDelim[5] = "\r\n\r\n";
const char kDefaultDestFile[11] = "index.html";

nsHTTPConn::nsHTTPConn(char *aHost, int aPort, char *aPath, int (*aEventPumpCB)(void)): 
    mEventPumpCB(aEventPumpCB),
    mHost(aHost),
    mPath(aPath),
    mProxiedURL(NULL),
    mProxyUser(NULL),
    mProxyPswd(NULL),
    mDestFile(NULL),
    mHostPathAllocd(FALSE),
    mSocket(NULL)
{
    if (aPort <= 0)
        mPort = kHTTPPort;
    else
        mPort = aPort;

    DUMP(("mHost = %s\n", mHost));
    DUMP(("mPort = %d\n", mPort));
    DUMP(("mPath = %s\n", mPath));
}

nsHTTPConn::nsHTTPConn(char *aHost, int aPort, char *aPath) :
    mEventPumpCB(NULL),
    mHost(aHost),
    mPath(aPath),
    mProxiedURL(NULL),
    mProxyUser(NULL),
    mProxyPswd(NULL),
    mDestFile(NULL),
    mHostPathAllocd(FALSE),
    mSocket(NULL)
{
    if (aPort <= 0)
        mPort = kHTTPPort;
    else
        mPort = aPort;

    DUMP(("mHost = %s\n", mHost));
    DUMP(("mPort = %d\n", mPort));
    DUMP(("mPath = %s\n", mPath));
}

nsHTTPConn::nsHTTPConn(char *aURL, int (*aEventPumpCB)(void)) :
    mEventPumpCB(aEventPumpCB),
    mPort(kHTTPPort),
    mProxiedURL(NULL),
    mProxyUser(NULL),
    mProxyPswd(NULL),
    mDestFile(NULL),
    mHostPathAllocd(FALSE),
    mSocket(NULL),
    mResponseCode(0)
{
    
    if (ParseURL(kHTTPProto, aURL, &mHost, &mPort, &mPath) == OK)
        mHostPathAllocd = TRUE;
    else
    {
        mHost = NULL;
        mPath = NULL;
    }

    DUMP(("mHost = %s\n", mHost));
    DUMP(("mPort = %d\n", mPort));
    DUMP(("mPath = %s\n", mPath));
}

nsHTTPConn::nsHTTPConn(char *aURL) :
    mEventPumpCB(NULL),
    mPort(kHTTPPort),
    mProxiedURL(NULL),
    mProxyUser(NULL),
    mProxyPswd(NULL),
    mDestFile(NULL),
    mHostPathAllocd(FALSE),
    mSocket(NULL),
    mResponseCode(0)
{
    
    if (ParseURL(kHTTPProto, aURL, &mHost, &mPort, &mPath) == OK)
        mHostPathAllocd = TRUE;
    else
    {
        mHost = NULL;
        mPath = NULL;
    }

    DUMP(("mHost = %s\n", mHost));
    DUMP(("mPort = %d\n", mPort));
    DUMP(("mPath = %s\n", mPath));
}

nsHTTPConn::~nsHTTPConn()
{
    if (mHostPathAllocd)
    {
        if (mHost)
            free(mHost);
        if (mPath)
            free(mPath);
    }
}
    
int
nsHTTPConn::Open()
{
    
    if (!mHost || !mPath)
        return E_MALFORMED_URL;

    
    mSocket = new nsSocket(mHost, mPort, mEventPumpCB);
    if (!mSocket)
        return E_MEM;

    
    return mSocket->Open();
}

int
nsHTTPConn::ResumeOrGet(HTTPGetCB aCallback, char *aDestFile)
{
    struct stat stbuf;
    int rv = 0;
    int resPos = 0;
    
    if (!aDestFile)
        return E_PARAM;

    
    rv = stat(aDestFile, &stbuf);
    if (rv == 0)
        resPos = stbuf.st_size;

    return Get(aCallback, aDestFile, resPos);

    
    
}

int 
nsHTTPConn::Get(HTTPGetCB aCallback, char *aDestFile)
{
    

    return ResumeOrGet(aCallback, aDestFile);
}

int
nsHTTPConn::Get(HTTPGetCB aCallback, char *aDestFile, int aResumePos)
{
    int rv;
    char *pathToUse;

    
    if (!mHost || !mPath)
        return E_MALFORMED_URL;

    if (!aDestFile)
    {
        if (mProxiedURL)
            pathToUse = mProxiedURL;
        else
            pathToUse = mPath;

        
        if (*(pathToUse + strlen(pathToUse) - 1) == '/')
            aDestFile = (char *) kDefaultDestFile;
        else
            aDestFile = strrchr(pathToUse, '/') + 1;
    }

    
    rv = Request(aResumePos);

    
    if (rv == OK)
        rv = Response(aCallback, aDestFile, aResumePos);

    return rv;
}

int
nsHTTPConn::Close()
{
    int rv;

    
    rv = mSocket->Close();

    
    delete mSocket;

    return rv;
}

void
nsHTTPConn::SetProxyInfo(char *aProxiedURL, char *aProxyUser, 
                                            char *aProxyPswd)
{
    mProxiedURL = aProxiedURL;
    mProxyUser = aProxyUser;
    mProxyPswd = aProxyPswd;
}

int
nsHTTPConn::Request(int aResumePos)
{
    char req[kReqBufSize];
    char hdr[kHdrBufSize];
    int rv;

    memset(req, 0, kReqBufSize);

    

    
    memset(hdr, 0, kHdrBufSize);
    if (mProxiedURL)
    {
        char *host = NULL, *path = NULL;
        char proto[8];
        int port;
#ifdef DEBUG
        assert(sizeof hdr > (strlen(mProxiedURL) + 15 ));
#endif
        sprintf(hdr, "GET %s HTTP/1.0%s", mProxiedURL, kCRLF);
        strcpy(req, hdr);
        if (strncmp(mProxiedURL, kFTPProto, strlen(kFTPProto)) == 0) 
        {
            strcpy(proto,kFTPProto);
            port = kFTPPort;
        } 
        else 
        {
            strcpy(proto,kHTTPProto);
            port = kHTTPPort;
        }
        rv = ParseURL(proto, mProxiedURL,
                      &host, &port, &path);
        if (rv == OK) {
            memset(hdr, 0, kHdrBufSize);
            sprintf(hdr, "Host: %s:%d%s", host, port, kCRLF);
            strcat(req, hdr);
        }
        if (host)
            free(host);
        if (path)
            free(path);
    }
    else
    {
        sprintf(hdr, "GET %s HTTP/1.0%s", mPath, kCRLF);
        strcpy(req, hdr);

        memset(hdr, 0, kHdrBufSize);
        sprintf(hdr, "Host: %s%s", mHost, kCRLF);
        strcat(req, hdr);
    }

    
    if (mProxyUser && mProxyPswd)
    {
        char *usrPsd = (char *) malloc(strlen(mProxyUser) +
                                       strlen(":")        +
                                       strlen(mProxyPswd) + 1);
        if (!usrPsd)
            return E_MEM;
        sprintf(usrPsd, "%s:%s", mProxyUser, mProxyPswd);

        
        char usrPsdEncoded[128];  
        memset(usrPsdEncoded, 0, 128);

        DUMP(("Unencoded string: %s\n", usrPsd));
        rv = Base64Encode((const unsigned char *)usrPsd, strlen(usrPsd),
                          usrPsdEncoded, 128);
        free(usrPsd);
        DUMP(("Encoded string: %s\n", usrPsdEncoded));
        DUMP(("Base64Encode returned: %d\n", rv));
        if (rv <= 0)
        {
            return E_B64_ENCODE;
        }

        
        memset(hdr, 0, kHdrBufSize);
        sprintf(hdr, "Proxy-authorization: Basic %s%s", usrPsdEncoded, kCRLF);
        strcat(req, hdr);

        
        
    }

    
    if (aResumePos > 0)
    {
        sprintf(hdr, "Range: bytes=%d-%s", aResumePos, kCRLF);
        strcat(req, hdr);
    }

    
    strcat(req, kCRLF);

    
    int bufSize = strlen(req);
    rv = mSocket->Send((unsigned char *) req, &bufSize);
    DUMP(("\n\n%s", req));

    if (bufSize != (int) strlen(req))
        rv = E_REQ_INCOMPLETE;

    return rv;
}

int 
nsHTTPConn::Response(HTTPGetCB aCallback, char *aDestFile, int aResumePos)
{
    

    int rv = OK;
    char resp[kRespBufSize];
    int bufSize, total = 0, fwriteLen, fwrote, bytesWritten = 0, expectedSize = 0;
    FILE *destFd;
    char *fwritePos;
    int bFirstIter = TRUE;

    if (!aDestFile)
        return E_PARAM;

    
    if (aResumePos > 0)
    {
        destFd = fopen(aDestFile, "r+b");
        if (!destFd)
            return E_OPEN_FILE;

        if (fseek(destFd, aResumePos, SEEK_SET) != 0)
        {
            fclose(destFd);
            return E_SEEK_FILE;
        }
    }
    else
    {
        destFd = fopen(aDestFile, "w+b");
        if (!destFd)
            return E_OPEN_FILE;
    }

    
    do
    {
        memset(resp, 0, kRespBufSize);
        bufSize = kRespBufSize;       
        
        rv = mSocket->Recv((unsigned char *) resp, &bufSize);
        DUMP(("nsSocket::Recv returned: %d\t and recd: %d\n", rv, bufSize));

        if(rv == nsSocket::E_EOF_FOUND || (rv != nsSocket::E_READ_MORE && rv != nsSocket::OK) ) {
            break;
        }

        if (bFirstIter)
        {
            fwritePos = strstr(resp, kHdrBodyDelim);
            if (fwritePos == NULL)
            {
                
                fwritePos = resp;
                fwriteLen = bufSize;
            }
            else
            {
                ParseResponseCode((const char *)resp, &mResponseCode);
                
                if ( mResponseCode < 200 || mResponseCode >=300 )
                {
                  
                  
                  rv = nsHTTPConn::E_HTTP_RESPONSE;
                  break;
                }

                ParseContentLength((const char *)resp, &expectedSize);

                
                fwritePos += strlen(kHdrBodyDelim);
                fwriteLen = bufSize - (fwritePos - resp);
                total = expectedSize + aResumePos;
            }

            bFirstIter = FALSE;
        }
        else
        {
            fwritePos = resp;
            fwriteLen = bufSize;
        }

        fwrote = fwrite(fwritePos, sizeof(char), fwriteLen, destFd);
        assert(fwrote == fwriteLen);

        if (fwriteLen > 0)
            bytesWritten += fwriteLen;
        if (aCallback && 
           (aCallback(bytesWritten, total) == E_USER_CANCEL))
              rv = E_USER_CANCEL; 
                                  

        if ( mEventPumpCB )
            mEventPumpCB();

    } while ( rv == nsSocket::E_READ_MORE || rv == nsSocket::OK);
    
    if ( bytesWritten == expectedSize && rv != nsHTTPConn::E_HTTP_RESPONSE)
        rv = nsSocket::E_EOF_FOUND;
        
    if (rv == nsSocket::E_EOF_FOUND)
    {
        DUMP(("EOF detected\n"));
        rv = OK;
    }

    fclose(destFd);

    return rv;
}

int
nsHTTPConn::ParseURL(const char *aProto, char *aURL, char **aHost, 
                     int *aPort, char **aPath)
{
    if (!aURL || !aHost || !aPort || !aPath || !aProto)
        return E_PARAM;

    char *pos, *nextSlash, *nextColon, *end, *hostEnd;
    int protoLen = strlen(aProto);

    if ((strncmp(aURL, aProto, protoLen) != 0) ||
        (strlen(aURL) < 9))
        return E_MALFORMED_URL;

    pos = aURL + protoLen;
    nextColon = strchr(pos, ':');
    nextSlash = strchr(pos, '/');

    
    if (nextColon && ((nextSlash && nextColon < nextSlash) || 
                       !nextSlash))
    {
        int portStrLen;
        if (nextSlash)
            portStrLen = nextSlash - nextColon;
        else
            portStrLen = strlen(nextColon);

        char *portStr = (char *) malloc(portStrLen + 1);
        if (!portStr)
            return E_MEM;
        memset(portStr, 0, portStrLen + 1);
        strncpy(portStr, nextColon+1, portStrLen);
        *aPort = atoi(portStr);
        free(portStr);
    }
    if ( (!nextColon || (nextSlash && (nextColon > nextSlash)))
         && *aPort <= 0) 
        *aPort = -1;

    
    if (!nextSlash)
    {
        int copyLen;
        if (nextColon)
            copyLen = nextColon - pos;
        else
            copyLen = strlen(pos);

        *aHost = (char *) malloc(copyLen + 1); 
        if (!aHost)
            return E_MEM;
        memset(*aHost, 0, copyLen + 1);
        strncpy(*aHost, pos, copyLen);

        *aPath = (char *) malloc(2);
        strcpy(*aPath, "/");

        return OK;
    }
    
    
    if (nextColon)
        hostEnd = nextColon;
    else
        hostEnd = nextSlash;
    *aHost = (char *) malloc(hostEnd - pos + 1); 
    if (!*aHost)
        return E_MEM;
		memset(*aHost, 0, hostEnd - pos + 1);
    strncpy(*aHost, pos, hostEnd - pos);
    *(*aHost + (hostEnd - pos)) = 0; 

    pos = nextSlash;
    end = aURL + strlen(aURL);

    *aPath = (char *) malloc(end - pos + 1);
    if (!*aPath)
    {
        if (*aHost)
            free(*aHost);
        return E_MEM;
    }
    memset(*aPath, 0, end - pos + 1);
    strncpy(*aPath, pos, end - pos);

    return OK;
}

void
nsHTTPConn::ParseResponseCode(const char *aBuf, int *aCode)
{
  char codeStr[4];
  const char *pos;

  if (!aBuf || !aCode)
    return;

  
  if (strncmp(aBuf,"HTTP/",5) == 0)
  {
    pos = strstr(aBuf," ");  
    ++pos;                   
    strncpy((char *)codeStr,pos, 3);
    codeStr[3] = '\0';
    *aCode = atoi(codeStr);
  }
}

void
nsHTTPConn::ParseContentLength(const char *aBuf, int *aLength)
{
    const char *clHdr; 
    const char *eol, *pos;
    char clNameStr1[16] = "Content-length:";
    char clNameStr2[16] = "Content-Length:";
    char *clNameStr = clNameStr1;

    if (!aBuf || !aLength)
        return;  
    *aLength = 0;

    
    clHdr = strstr(aBuf, (char *)clNameStr1);
    if (!clHdr)
    {
        clHdr = strstr(aBuf, (char *)clNameStr2);
        clNameStr = clNameStr2;
    }
    if (clHdr)
    {
        eol = strstr(clHdr, kCRLF); 
        pos = clHdr + strlen(clNameStr);
        while ((pos < eol) && (*pos == ' ' || *pos == '\t'))
            pos++;
        if (pos < eol)
        {
            int clValStrLen = eol - pos + 1; 
            char *clValStr = (char *) malloc(clValStrLen);
            if (!clValStr)
                return; 

            memset(clValStr, 0, clValStrLen);
            strncpy(clValStr, pos, eol - pos);
            *aLength = atoi(clValStr);
        }
    } 
}

int 
nsHTTPConn::Base64Encode(const unsigned char *in_str, int in_len, 
                         char *out_str, int out_len)
{
    

    static unsigned char base64[] =
    {  
       
        'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 
        'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P',  
        'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X',  
        'Y', 'Z', 'a', 'b', 'c', 'd', 'e', 'f',  
        'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n',  
        'o', 'p', 'q', 'r', 's', 't', 'u', 'v',  
        'w', 'x', 'y', 'z', '0', '1', '2', '3', 
        '4', '5', '6', '7', '8', '9', '+', '/'   
    };

    int curr_out_len = 0;

    int i = 0;
    unsigned char a, b, c;

    out_str[0] = '\0';
    
    if (in_len > 0)
    {

        while (i < in_len)
        {
            a = in_str[i];
            b = (i + 1 >= in_len) ? 0 : in_str[i + 1];
            c = (i + 2 >= in_len) ? 0 : in_str[i + 2];

            if (i + 2 < in_len)
            {
                out_str[curr_out_len++] = (base64[(a >> 2) & 0x3F]);
                out_str[curr_out_len++] = (base64[((a << 4) & 0x30)
                                            + ((b >> 4) & 0xf)]);
                out_str[curr_out_len++] = (base64[((b << 2) & 0x3c)
                                            + ((c >> 6) & 0x3)]);
                out_str[curr_out_len++] = (base64[c & 0x3F]);
            }
            else if (i + 1 < in_len)
            {
                out_str[curr_out_len++] = (base64[(a >> 2) & 0x3F]);
                out_str[curr_out_len++] = (base64[((a << 4) & 0x30)
                                            + ((b >> 4) & 0xf)]);
                out_str[curr_out_len++] = (base64[((b << 2) & 0x3c)
                                            + ((c >> 6) & 0x3)]);
                out_str[curr_out_len++] = '=';
            }
            else
            {
                out_str[curr_out_len++] = (base64[(a >> 2) & 0x3F]);
                out_str[curr_out_len++] = (base64[((a << 4) & 0x30)
                                            + ((b >> 4) & 0xf)]);
                out_str[curr_out_len++] = '=';
                out_str[curr_out_len++] = '=';
            }

            i += 3;

            if ((curr_out_len + 4) > out_len)
            {
                return(-1);
            }

        }
        out_str[curr_out_len] = '\0';
    }
    
    return curr_out_len;
}

#ifdef TEST_NSHTTPCONN

int
TestHTTPCB(int aBytesRd, int aTotal)
{
    DUMP(("Bytes rd: %d\tTotal: %d\n", aBytesRd, aTotal));
    return 0;
}

int
main(int argc, char **argv)
{
    nsHTTPConn *conn;
    int rv = nsHTTPConn::OK;
    char *proxiedURL = NULL;
    char *proxyUser = NULL;
    char *proxyPswd = NULL;

    DUMP(("*** %s: A self-test for the nsHTTPConn class.\n", argv[0]));

    if (argc < 2)
    {
        printf("usage: %s <http_url> [<proxied_url> [<proxy_user> ", argv[0]);
        printf("<proxy_pswd>]]\n");
        exit(1);
    }

    conn = new nsHTTPConn(argv[1]);

    if (argc >= 3)
    {
        proxiedURL = argv[2];
    }
    if (argc >= 5)
    {
        proxyUser = argv[3];
        proxyPswd = argv[4];
    }

    conn->SetProxyInfo(proxiedURL, proxyUser, proxyPswd);

    rv = conn->Open();
    DUMP(("nsHTTPConn::Open returned: %d\n", rv));

    rv = conn->Get(TestHTTPCB, NULL); 
    DUMP(("nsHTTPConn::Get returned: %d\n", rv));

    rv = conn->Close();
    DUMP(("nsHTTPConn::Close returned: %d\n", rv));

    return 0;
}

#endif 
