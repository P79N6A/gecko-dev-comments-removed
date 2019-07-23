










































#ifndef pprio_h___
#define pprio_h___

#include "prtypes.h"
#include "prio.h"

PR_BEGIN_EXTERN_C




#ifdef _WIN64
typedef __int64 PROsfd;
#else
typedef PRInt32 PROsfd;
#endif


NSPR_API(const PRIOMethods*)    PR_GetFileMethods(void);
NSPR_API(const PRIOMethods*)    PR_GetTCPMethods(void);
NSPR_API(const PRIOMethods*)    PR_GetUDPMethods(void);
NSPR_API(const PRIOMethods*)    PR_GetPipeMethods(void);

















NSPR_API(PROsfd)       PR_FileDesc2NativeHandle(PRFileDesc *);
NSPR_API(void)         PR_ChangeFileDescNativeHandle(PRFileDesc *, PROsfd);
NSPR_API(PRFileDesc*)  PR_AllocFileDesc(PROsfd osfd,
                                         const PRIOMethods *methods);
NSPR_API(void)         PR_FreeFileDesc(PRFileDesc *fd);



NSPR_API(PRFileDesc*)  PR_ImportFile(PROsfd osfd);
NSPR_API(PRFileDesc*)  PR_ImportPipe(PROsfd osfd);
NSPR_API(PRFileDesc*)  PR_ImportTCPSocket(PROsfd osfd);
NSPR_API(PRFileDesc*)  PR_ImportUDPSocket(PROsfd osfd);




















NSPR_API(PRFileDesc*)	PR_CreateSocketPollFd(PROsfd osfd);

















NSPR_API(PRStatus) PR_DestroySocketPollFd(PRFileDesc *fd);








#ifdef WIN32

#define PR_SOCK_STREAM 1
#define PR_SOCK_DGRAM 2

#else 

#define PR_SOCK_STREAM SOCK_STREAM
#define PR_SOCK_DGRAM SOCK_DGRAM

#endif 




NSPR_API(PRFileDesc*)	PR_Socket(PRInt32 domain, PRInt32 type, PRInt32 proto);








NSPR_API(PRStatus) PR_LockFile(PRFileDesc *fd);









NSPR_API(PRStatus) PR_TLockFile(PRFileDesc *fd);









NSPR_API(PRStatus) PR_UnlockFile(PRFileDesc *fd);




NSPR_API(PRInt32) PR_EmulateAcceptRead(PRFileDesc *sd, PRFileDesc **nd,
    PRNetAddr **raddr, void *buf, PRInt32 amount, PRIntervalTime timeout);





NSPR_API(PRInt32) PR_EmulateSendFile(
    PRFileDesc *networkSocket, PRSendFileData *sendData,
    PRTransmitFileFlags flags, PRIntervalTime timeout);

#ifdef WIN32








NSPR_API(PRInt32) PR_NTFast_AcceptRead(PRFileDesc *sd, PRFileDesc **nd,
              PRNetAddr **raddr, void *buf, PRInt32 amount, PRIntervalTime t);

typedef void (*_PR_AcceptTimeoutCallback)(void *);















NSPR_API(PRInt32) PR_NTFast_AcceptRead_WithTimeoutCallback(
              PRFileDesc *sd, 
              PRFileDesc **nd,
              PRNetAddr **raddr, 
              void *buf, 
              PRInt32 amount, 
              PRIntervalTime t,
              _PR_AcceptTimeoutCallback callback, 
              void *callback_arg);









NSPR_API(PRFileDesc*)	PR_NTFast_Accept(PRFileDesc *fd, PRNetAddr *addr,
                                                PRIntervalTime timeout);









NSPR_API(void) PR_NTFast_UpdateAcceptContext(PRFileDesc *acceptSock, 
                                        PRFileDesc *listenSock);






NSPR_API(PRStatus) PR_NT_CancelIo(PRFileDesc *fd);


#endif 

PR_END_EXTERN_C

#endif 
