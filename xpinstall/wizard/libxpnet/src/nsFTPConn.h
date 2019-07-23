






































#ifndef _NS_FTPCONN_H_
#define _NS_FTPCONN_H_

class nsSocket; 

typedef int (*FTPGetCB)(int aBytesRd, int aTotal);

class nsFTPConn
{
public:
    nsFTPConn(char *aHost);
    nsFTPConn(char *aHost, int (*aEventPumpCB)(void));
    ~nsFTPConn();

    
    enum
    {
        ASCII = 0,
        BINARY
    };

    
    enum
    {
        OPEN = 0,
        GETTING,
        CLOSED
    };

    int     Open();
    int     Open(char *aHost);
    int     ResumeOrGet(char *aSrvPath, char *aLoclPath, int aType, 
                int aOvWrite, FTPGetCB aCBFunc);
    int     Get(char *aSrvPath, char *aLoclPath, int aType, 
                int aOvWrite, FTPGetCB aCBFunc);
    int     Get(char *aSrvPath, char *aLoclPath, int aType, int aResumePos,
                int aOvWrite, FTPGetCB aCBFunc);
    int     Close();




    enum
    {
        OK                  = 0,
        E_MEM               = -801, 
        E_PARAM             = -802, 
        E_ALREADY_OPEN      = -803, 
        E_NOT_OPEN          = -804, 
        E_CMD_ERR           = -805, 
        E_CMD_FAIL          = -806, 
        E_CMD_UNEXPECTED    = -807, 
        E_WRITE             = -808, 
        E_READ              = -809, 
        E_SMALL_BUF         = -810, 
        E_CANT_OVWRITE      = -811, 
        E_LOCL_INIT         = -812, 
        E_USER_CANCEL       = -813, 
        E_INVALID_ADDR      = -814  
    };

private:
    int         FlushCntlSock(nsSocket *aSock, int bailOnTimeOut = 1);
    int         IssueCmd(const char *aCmd, char *aResp, int aRespSize, 
                         nsSocket *aSock);
    int         ParseAddr(char *aBuf, char **aHost, int *aPort);
    int         DataInit(char *aHost, int aPort, nsSocket **aSock); 

    int         (*mEventPumpCB)(void);
    char        *mHost;
    int         mState;
    int         mPassive;
    nsSocket    *mCntlSock;
    nsSocket    *mDataSock;
};

#ifndef NULL
#define NULL 0
#endif

#ifndef TRUE
#define TRUE 1
#endif

#ifndef FALSE
#define FALSE 0
#endif

#ifdef DUMP
#undef DUMP
#endif 

#if defined(DEBUG) || defined(DEBUG_sgehani)
#define DUMP(_msg) printf("%s %d: %s\n", __FILE__, __LINE__, _msg);
#else
#define DUMP(_msg)  
#endif 

#ifndef ERR_CHECK
#define ERR_CHECK(_func)                                            \
do {                                                                \
    err = _func;                                                    \
    if (err != OK)                                                  \
        goto BAIL;                                                  \
} while(0);

#endif

#endif 

