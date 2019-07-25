











































#ifndef prio_h___
#define prio_h___

#include "prlong.h"
#include "prtime.h"
#include "prinrval.h"
#include "prinet.h"

PR_BEGIN_EXTERN_C


typedef struct PRDir            PRDir;
typedef struct PRDirEntry       PRDirEntry;
#ifdef MOZ_UNICODE
typedef struct PRDirUTF16       PRDirUTF16;
typedef struct PRDirEntryUTF16  PRDirEntryUTF16;
#endif 
typedef struct PRFileDesc       PRFileDesc;
typedef struct PRFileInfo       PRFileInfo;
typedef struct PRFileInfo64     PRFileInfo64;
typedef union  PRNetAddr        PRNetAddr;
typedef struct PRIOMethods      PRIOMethods;
typedef struct PRPollDesc       PRPollDesc;
typedef struct PRFilePrivate    PRFilePrivate;
typedef struct PRSendFileData   PRSendFileData;












typedef PRIntn PRDescIdentity;          

struct PRFileDesc {
    const PRIOMethods *methods;         
    PRFilePrivate *secret;              
    PRFileDesc *lower, *higher;         
    void (PR_CALLBACK *dtor)(PRFileDesc *fd);
                                        
    PRDescIdentity identity;            
};










typedef enum PRTransmitFileFlags {
    PR_TRANSMITFILE_KEEP_OPEN = 0,    

    PR_TRANSMITFILE_CLOSE_SOCKET = 1  

} PRTransmitFileFlags;










#ifdef WIN32

#define PR_AF_INET 2
#define PR_AF_LOCAL 1
#define PR_INADDR_ANY (unsigned long)0x00000000
#define PR_INADDR_LOOPBACK 0x7f000001
#define PR_INADDR_BROADCAST (unsigned long)0xffffffff

#else 

#define PR_AF_INET AF_INET
#define PR_AF_LOCAL AF_UNIX
#define PR_INADDR_ANY INADDR_ANY
#define PR_INADDR_LOOPBACK INADDR_LOOPBACK
#define PR_INADDR_BROADCAST INADDR_BROADCAST

#endif 






#ifndef PR_AF_INET6
#define PR_AF_INET6 100
#endif

#define PR_AF_INET_SDP 101
#define PR_AF_INET6_SDP 102

#ifndef PR_AF_UNSPEC
#define PR_AF_UNSPEC 0
#endif











struct PRIPv6Addr {
	union {
		PRUint8  _S6_u8[16];
		PRUint16 _S6_u16[8];
		PRUint32 _S6_u32[4];
		PRUint64 _S6_u64[2];
	} _S6_un;
};
#define pr_s6_addr		_S6_un._S6_u8
#define pr_s6_addr16	_S6_un._S6_u16
#define pr_s6_addr32	_S6_un._S6_u32
#define pr_s6_addr64 	_S6_un._S6_u64

typedef struct PRIPv6Addr PRIPv6Addr;

union PRNetAddr {
    struct {
        PRUint16 family;                
#ifdef XP_BEOS
        char data[10];                  
#else
        char data[14];                  
#endif
    } raw;
    struct {
        PRUint16 family;                
        PRUint16 port;                  
        PRUint32 ip;                    
#ifdef XP_BEOS
        char pad[4];                    
#else
        char pad[8];
#endif
    } inet;
    struct {
        PRUint16 family;                
        PRUint16 port;                  
        PRUint32 flowinfo;              
        PRIPv6Addr ip;                  
        PRUint32 scope_id;              
    } ipv6;
#if defined(XP_UNIX) || defined(XP_OS2)
    struct {                            
        PRUint16 family;                
#ifdef XP_OS2
        char path[108];                 
                                        
#else
        char path[104];                 
#endif
    } local;
#endif
};










typedef enum PRSockOption
{
    PR_SockOpt_Nonblocking,     
    PR_SockOpt_Linger,          
    PR_SockOpt_Reuseaddr,       
    PR_SockOpt_Keepalive,       
    PR_SockOpt_RecvBufferSize,  
    PR_SockOpt_SendBufferSize,  

    PR_SockOpt_IpTimeToLive,    
    PR_SockOpt_IpTypeOfService, 

    PR_SockOpt_AddMember,       
    PR_SockOpt_DropMember,      
    PR_SockOpt_McastInterface,  
    PR_SockOpt_McastTimeToLive, 
    PR_SockOpt_McastLoopback,   

    PR_SockOpt_NoDelay,         
    PR_SockOpt_MaxSegment,      
    PR_SockOpt_Broadcast,       
    PR_SockOpt_Last
} PRSockOption;

typedef struct PRLinger {
	PRBool polarity;		    
	PRIntervalTime linger;	    
} PRLinger;

typedef struct PRMcastRequest {
	PRNetAddr mcaddr;			
	PRNetAddr ifaddr;			
} PRMcastRequest;

typedef struct PRSocketOptionData
{
    PRSockOption option;
    union
    {
        PRUintn ip_ttl;             
        PRUintn mcast_ttl;          
        PRUintn tos;                
        PRBool non_blocking;        
        PRBool reuse_addr;          
        PRBool keep_alive;          
        PRBool mcast_loopback;      
        PRBool no_delay;            
        PRBool broadcast;           
        PRSize max_segment;         
        PRSize recv_buffer_size;    
        PRSize send_buffer_size;    
        PRLinger linger;            
        PRMcastRequest add_member;  
        PRMcastRequest drop_member; 
        PRNetAddr mcast_if;         
    } value;
} PRSocketOptionData;









typedef struct PRIOVec {
    char *iov_base;
    int iov_len;
} PRIOVec;






typedef enum PRDescType
{
    PR_DESC_FILE = 1,
    PR_DESC_SOCKET_TCP = 2,
    PR_DESC_SOCKET_UDP = 3,
    PR_DESC_LAYERED = 4,
    PR_DESC_PIPE = 5
} PRDescType;

typedef enum PRSeekWhence {
    PR_SEEK_SET = 0,
    PR_SEEK_CUR = 1,
    PR_SEEK_END = 2
} PRSeekWhence;

NSPR_API(PRDescType) PR_GetDescType(PRFileDesc *file);

















typedef PRStatus (PR_CALLBACK *PRCloseFN)(PRFileDesc *fd);
typedef PRInt32 (PR_CALLBACK *PRReadFN)(PRFileDesc *fd, void *buf, PRInt32 amount);
typedef PRInt32 (PR_CALLBACK *PRWriteFN)(PRFileDesc *fd, const void *buf, PRInt32 amount);
typedef PRInt32 (PR_CALLBACK *PRAvailableFN)(PRFileDesc *fd);
typedef PRInt64 (PR_CALLBACK *PRAvailable64FN)(PRFileDesc *fd);
typedef PRStatus (PR_CALLBACK *PRFsyncFN)(PRFileDesc *fd);
typedef PROffset32 (PR_CALLBACK *PRSeekFN)(PRFileDesc *fd, PROffset32 offset, PRSeekWhence how);
typedef PROffset64 (PR_CALLBACK *PRSeek64FN)(PRFileDesc *fd, PROffset64 offset, PRSeekWhence how);
typedef PRStatus (PR_CALLBACK *PRFileInfoFN)(PRFileDesc *fd, PRFileInfo *info);
typedef PRStatus (PR_CALLBACK *PRFileInfo64FN)(PRFileDesc *fd, PRFileInfo64 *info);
typedef PRInt32 (PR_CALLBACK *PRWritevFN)(
    PRFileDesc *fd, const PRIOVec *iov, PRInt32 iov_size,
    PRIntervalTime timeout);
typedef PRStatus (PR_CALLBACK *PRConnectFN)(
    PRFileDesc *fd, const PRNetAddr *addr, PRIntervalTime timeout);
typedef PRFileDesc* (PR_CALLBACK *PRAcceptFN) (
    PRFileDesc *fd, PRNetAddr *addr, PRIntervalTime timeout);
typedef PRStatus (PR_CALLBACK *PRBindFN)(PRFileDesc *fd, const PRNetAddr *addr);
typedef PRStatus (PR_CALLBACK *PRListenFN)(PRFileDesc *fd, PRIntn backlog);
typedef PRStatus (PR_CALLBACK *PRShutdownFN)(PRFileDesc *fd, PRIntn how);
typedef PRInt32 (PR_CALLBACK *PRRecvFN)(
    PRFileDesc *fd, void *buf, PRInt32 amount,
    PRIntn flags, PRIntervalTime timeout);
typedef PRInt32 (PR_CALLBACK *PRSendFN) (
    PRFileDesc *fd, const void *buf, PRInt32 amount,
    PRIntn flags, PRIntervalTime timeout);
typedef PRInt32 (PR_CALLBACK *PRRecvfromFN)(
    PRFileDesc *fd, void *buf, PRInt32 amount,
    PRIntn flags, PRNetAddr *addr, PRIntervalTime timeout);
typedef PRInt32 (PR_CALLBACK *PRSendtoFN)(
    PRFileDesc *fd, const void *buf, PRInt32 amount,
    PRIntn flags, const PRNetAddr *addr, PRIntervalTime timeout);
typedef PRInt16 (PR_CALLBACK *PRPollFN)(
    PRFileDesc *fd, PRInt16 in_flags, PRInt16 *out_flags);
typedef PRInt32 (PR_CALLBACK *PRAcceptreadFN)(
    PRFileDesc *sd, PRFileDesc **nd, PRNetAddr **raddr,
    void *buf, PRInt32 amount, PRIntervalTime t);
typedef PRInt32 (PR_CALLBACK *PRTransmitfileFN)(
     PRFileDesc *sd, PRFileDesc *fd, const void *headers,
     PRInt32 hlen, PRTransmitFileFlags flags, PRIntervalTime t);
typedef PRStatus (PR_CALLBACK *PRGetsocknameFN)(PRFileDesc *fd, PRNetAddr *addr);
typedef PRStatus (PR_CALLBACK *PRGetpeernameFN)(PRFileDesc *fd, PRNetAddr *addr);
typedef PRStatus (PR_CALLBACK *PRGetsocketoptionFN)(
    PRFileDesc *fd, PRSocketOptionData *data);
typedef PRStatus (PR_CALLBACK *PRSetsocketoptionFN)(
    PRFileDesc *fd, const PRSocketOptionData *data);
typedef PRInt32 (PR_CALLBACK *PRSendfileFN)(
	PRFileDesc *networkSocket, PRSendFileData *sendData,
	PRTransmitFileFlags flags, PRIntervalTime timeout);
typedef PRStatus (PR_CALLBACK *PRConnectcontinueFN)(
    PRFileDesc *fd, PRInt16 out_flags);
typedef PRIntn (PR_CALLBACK *PRReservedFN)(PRFileDesc *fd);

struct PRIOMethods {
    PRDescType file_type;           
    PRCloseFN close;                
    PRReadFN read;                  
    PRWriteFN write;                
    PRAvailableFN available;        
    PRAvailable64FN available64;    
    PRFsyncFN fsync;                
    PRSeekFN seek;                  
    PRSeek64FN seek64;              
    PRFileInfoFN fileInfo;          
    PRFileInfo64FN fileInfo64;      
    PRWritevFN writev;              
    PRConnectFN connect;            
    PRAcceptFN accept;              
    PRBindFN bind;                  
    PRListenFN listen;              
    PRShutdownFN shutdown;          
    PRRecvFN recv;                  
    PRSendFN send;                  
    PRRecvfromFN recvfrom;          
    PRSendtoFN sendto;              
    PRPollFN poll;                  
    PRAcceptreadFN acceptread;      
    PRTransmitfileFN transmitfile;  
    PRGetsocknameFN getsockname;    
    PRGetpeernameFN getpeername;    
    PRReservedFN reserved_fn_6;     
    PRReservedFN reserved_fn_5;     
    PRGetsocketoptionFN getsocketoption;
                                    
    PRSetsocketoptionFN setsocketoption;
                                    
    PRSendfileFN sendfile;			
    PRConnectcontinueFN connectcontinue;
                                    
    PRReservedFN reserved_fn_3;		
    PRReservedFN reserved_fn_2;		
    PRReservedFN reserved_fn_1;		
    PRReservedFN reserved_fn_0;		
};




















typedef enum PRSpecialFD
{
    PR_StandardInput,          
    PR_StandardOutput,         
    PR_StandardError           
} PRSpecialFD;

NSPR_API(PRFileDesc*) PR_GetSpecialFD(PRSpecialFD id);

#define PR_STDIN	PR_GetSpecialFD(PR_StandardInput)
#define PR_STDOUT	PR_GetSpecialFD(PR_StandardOutput)
#define PR_STDERR	PR_GetSpecialFD(PR_StandardError)






























#define PR_IO_LAYER_HEAD (PRDescIdentity)-3
#define PR_INVALID_IO_LAYER (PRDescIdentity)-1
#define PR_TOP_IO_LAYER (PRDescIdentity)-2
#define PR_NSPR_IO_LAYER (PRDescIdentity)0

NSPR_API(PRDescIdentity) PR_GetUniqueIdentity(const char *layer_name);
NSPR_API(const char*) PR_GetNameForIdentity(PRDescIdentity ident);
NSPR_API(PRDescIdentity) PR_GetLayersIdentity(PRFileDesc* fd);
NSPR_API(PRFileDesc*) PR_GetIdentitiesLayer(PRFileDesc* fd_stack, PRDescIdentity id);









NSPR_API(const PRIOMethods *) PR_GetDefaultIOMethods(void);










NSPR_API(PRFileDesc*) PR_CreateIOLayerStub(
    PRDescIdentity ident, const PRIOMethods *methods);















NSPR_API(PRFileDesc*) PR_CreateIOLayer(PRFileDesc* fd);















NSPR_API(PRStatus) PR_PushIOLayer(
    PRFileDesc *fd_stack, PRDescIdentity id, PRFileDesc *layer);















NSPR_API(PRFileDesc*) PR_PopIOLayer(PRFileDesc *fd_stack, PRDescIdentity id);














































#define PR_RDONLY       0x01
#define PR_WRONLY       0x02
#define PR_RDWR         0x04
#define PR_CREATE_FILE  0x08
#define PR_APPEND       0x10
#define PR_TRUNCATE     0x20
#define PR_SYNC         0x40
#define PR_EXCL         0x80



















NSPR_API(PRFileDesc*) PR_Open(const char *name, PRIntn flags, PRIntn mode);












#define PR_IRWXU 00700  /* read, write, execute/search by owner */
#define PR_IRUSR 00400  /* read permission, owner */
#define PR_IWUSR 00200  /* write permission, owner */
#define PR_IXUSR 00100  /* execute/search permission, owner */
#define PR_IRWXG 00070  /* read, write, execute/search by group */
#define PR_IRGRP 00040  /* read permission, group */
#define PR_IWGRP 00020  /* write permission, group */
#define PR_IXGRP 00010  /* execute/search permission, group */
#define PR_IRWXO 00007  /* read, write, execute/search by others */
#define PR_IROTH 00004  /* read permission, others */
#define PR_IWOTH 00002  /* write permission, others */
#define PR_IXOTH 00001  /* execute/search permission, others */

NSPR_API(PRFileDesc*) PR_OpenFile(
    const char *name, PRIntn flags, PRIntn mode);

#ifdef MOZ_UNICODE



NSPR_API(PRFileDesc*) PR_OpenFileUTF16(
    const PRUnichar *name, PRIntn flags, PRIntn mode);
#endif 





















NSPR_API(PRStatus)    PR_Close(PRFileDesc *fd);


































NSPR_API(PRInt32) PR_Read(PRFileDesc *fd, void *buf, PRInt32 amount);























NSPR_API(PRInt32) PR_Write(PRFileDesc *fd,const void *buf,PRInt32 amount);































#define PR_MAX_IOVECTOR_SIZE 16   /* 'iov_size' must be <= */

NSPR_API(PRInt32) PR_Writev(
    PRFileDesc *fd, const PRIOVec *iov, PRInt32 iov_size,
    PRIntervalTime timeout);


















NSPR_API(PRStatus) PR_Delete(const char *name);



typedef enum PRFileType
{
    PR_FILE_FILE = 1,
    PR_FILE_DIRECTORY = 2,
    PR_FILE_OTHER = 3
} PRFileType;

struct PRFileInfo {
    PRFileType type;        
    PROffset32 size;        
    PRTime creationTime;    
    PRTime modifyTime;      
};

struct PRFileInfo64 {
    PRFileType type;        
    PROffset64 size;        
    PRTime creationTime;    
    PRTime modifyTime;      
};



















NSPR_API(PRStatus) PR_GetFileInfo(const char *fn, PRFileInfo *info);
NSPR_API(PRStatus) PR_GetFileInfo64(const char *fn, PRFileInfo64 *info);

#ifdef MOZ_UNICODE



NSPR_API(PRStatus) PR_GetFileInfo64UTF16(const PRUnichar *fn, PRFileInfo64 *info);
#endif 


















NSPR_API(PRStatus) PR_GetOpenFileInfo(PRFileDesc *fd, PRFileInfo *info);
NSPR_API(PRStatus) PR_GetOpenFileInfo64(PRFileDesc *fd, PRFileInfo64 *info);

















NSPR_API(PRStatus)    PR_Rename(const char *from, const char *to);

























typedef enum PRAccessHow {
    PR_ACCESS_EXISTS = 1,
    PR_ACCESS_WRITE_OK = 2,
    PR_ACCESS_READ_OK = 3
} PRAccessHow;

NSPR_API(PRStatus) PR_Access(const char *name, PRAccessHow how);


































NSPR_API(PROffset32) PR_Seek(PRFileDesc *fd, PROffset32 offset, PRSeekWhence whence);
NSPR_API(PROffset64) PR_Seek64(PRFileDesc *fd, PROffset64 offset, PRSeekWhence whence);





















NSPR_API(PRInt32) PR_Available(PRFileDesc *fd);
NSPR_API(PRInt64) PR_Available64(PRFileDesc *fd);


















NSPR_API(PRStatus)	PR_Sync(PRFileDesc *fd);



struct PRDirEntry {
    const char *name;        
};

#ifdef MOZ_UNICODE
struct PRDirEntryUTF16 {
    const PRUnichar *name;   

};
#endif 

#if !defined(NO_NSPR_10_SUPPORT)
#define PR_DirName(dirEntry)	(dirEntry->name)
#endif





















NSPR_API(PRDir*) PR_OpenDir(const char *name);

#ifdef MOZ_UNICODE



NSPR_API(PRDirUTF16*) PR_OpenDirUTF16(const PRUnichar *name);
#endif 
























typedef enum PRDirFlags {
    PR_SKIP_NONE = 0x0,
    PR_SKIP_DOT = 0x1,
    PR_SKIP_DOT_DOT = 0x2,
    PR_SKIP_BOTH = 0x3,
    PR_SKIP_HIDDEN = 0x4
} PRDirFlags;

NSPR_API(PRDirEntry*) PR_ReadDir(PRDir *dir, PRDirFlags flags);

#ifdef MOZ_UNICODE



NSPR_API(PRDirEntryUTF16*) PR_ReadDirUTF16(PRDirUTF16 *dir, PRDirFlags flags);
#endif 


















NSPR_API(PRStatus) PR_CloseDir(PRDir *dir);

#ifdef MOZ_UNICODE



NSPR_API(PRStatus) PR_CloseDirUTF16(PRDirUTF16 *dir);
#endif 





















NSPR_API(PRStatus) PR_MkDir(const char *name, PRIntn mode);











NSPR_API(PRStatus) PR_MakeDir(const char *name, PRIntn mode);



















NSPR_API(PRStatus) PR_RmDir(const char *name);


















NSPR_API(PRFileDesc*)    PR_NewUDPSocket(void);


















NSPR_API(PRFileDesc*)    PR_NewTCPSocket(void);



















NSPR_API(PRFileDesc*)    PR_OpenUDPSocket(PRIntn af);



















NSPR_API(PRFileDesc*)    PR_OpenTCPSocket(PRIntn af);



























NSPR_API(PRStatus) PR_Connect(
    PRFileDesc *fd, const PRNetAddr *addr, PRIntervalTime timeout);

































NSPR_API(PRStatus)    PR_ConnectContinue(PRFileDesc *fd, PRInt16 out_flags);





























NSPR_API(PRStatus)    PR_GetConnectStatus(const PRPollDesc *pd);























NSPR_API(PRFileDesc*) PR_Accept(
    PRFileDesc *fd, PRNetAddr *addr, PRIntervalTime timeout);




















NSPR_API(PRStatus) PR_Bind(PRFileDesc *fd, const PRNetAddr *addr);





















NSPR_API(PRStatus) PR_Listen(PRFileDesc *fd, PRIntn backlog);























typedef enum PRShutdownHow
{
    PR_SHUTDOWN_RCV = 0,      
    PR_SHUTDOWN_SEND = 1,     
    PR_SHUTDOWN_BOTH = 2      
} PRShutdownHow;

NSPR_API(PRStatus)    PR_Shutdown(PRFileDesc *fd, PRShutdownHow how);






























#define PR_MSG_PEEK 0x2

NSPR_API(PRInt32)    PR_Recv(PRFileDesc *fd, void *buf, PRInt32 amount,
                PRIntn flags, PRIntervalTime timeout);





























NSPR_API(PRInt32)    PR_Send(PRFileDesc *fd, const void *buf, PRInt32 amount,
                                PRIntn flags, PRIntervalTime timeout);

































NSPR_API(PRInt32) PR_RecvFrom(
    PRFileDesc *fd, void *buf, PRInt32 amount, PRIntn flags,
    PRNetAddr *addr, PRIntervalTime timeout);






























NSPR_API(PRInt32) PR_SendTo(
    PRFileDesc *fd, const void *buf, PRInt32 amount, PRIntn flags,
    const PRNetAddr *addr, PRIntervalTime timeout);





































NSPR_API(PRInt32) PR_TransmitFile(
    PRFileDesc *networkSocket, PRFileDesc *sourceFile,
    const void *headers, PRInt32 hlen, PRTransmitFileFlags flags,
    PRIntervalTime timeout);



































struct PRSendFileData {
	PRFileDesc	*fd;			
	PRUint32	file_offset;	
	PRSize		file_nbytes;	
								
								
	const void	*header;		
	PRInt32		hlen;			
	const void	*trailer;		
	PRInt32		tlen;			
};


NSPR_API(PRInt32) PR_SendFile(
    PRFileDesc *networkSocket, PRSendFileData *sendData,
	PRTransmitFileFlags flags, PRIntervalTime timeout);









































       







#define PR_ACCEPT_READ_BUF_OVERHEAD (32+(2*sizeof(PRNetAddr)))

NSPR_API(PRInt32) PR_AcceptRead(
    PRFileDesc *listenSock, PRFileDesc **acceptedSock,
    PRNetAddr **peerAddr, void *buf, PRInt32 amount, PRIntervalTime timeout);





















NSPR_API(PRStatus) PR_NewTCPSocketPair(PRFileDesc *fds[2]);



















NSPR_API(PRStatus)	PR_GetSockName(PRFileDesc *fd, PRNetAddr *addr);





















NSPR_API(PRStatus)	PR_GetPeerName(PRFileDesc *fd, PRNetAddr *addr);

NSPR_API(PRStatus)	PR_GetSocketOption(
    PRFileDesc *fd, PRSocketOptionData *data);

NSPR_API(PRStatus)	PR_SetSocketOption(
    PRFileDesc *fd, const PRSocketOptionData *data);




























NSPR_API(PRStatus) PR_SetFDInheritable(
    PRFileDesc *fd,
    PRBool inheritable);

















NSPR_API(PRFileDesc *) PR_GetInheritedFD(const char *name);









typedef struct PRFileMap PRFileMap;




typedef enum PRFileMapProtect {
    PR_PROT_READONLY,     
    PR_PROT_READWRITE,    
    PR_PROT_WRITECOPY     
} PRFileMapProtect;

NSPR_API(PRFileMap *) PR_CreateFileMap(
    PRFileDesc *fd,
    PRInt64 size,
    PRFileMapProtect prot);




NSPR_API(PRInt32) PR_GetMemMapAlignment(void);

NSPR_API(void *) PR_MemMap(
    PRFileMap *fmap,
    PROffset64 offset,  

    PRUint32 len);

NSPR_API(PRStatus) PR_MemUnmap(void *addr, PRUint32 len);

NSPR_API(PRStatus) PR_CloseFileMap(PRFileMap *fmap);














NSPR_API(PRStatus) PR_CreatePipe(
    PRFileDesc **readPipe,
    PRFileDesc **writePipe
);





struct PRPollDesc {
    PRFileDesc* fd;
    PRInt16 in_flags;
    PRInt16 out_flags;
};






#if defined(_PR_POLL_BACKCOMPAT)

#include <poll.h>
#define PR_POLL_READ    POLLIN
#define PR_POLL_WRITE   POLLOUT
#define PR_POLL_EXCEPT  POLLPRI
#define PR_POLL_ERR     POLLERR     /* only in out_flags */
#define PR_POLL_NVAL    POLLNVAL    /* only in out_flags when fd is bad */
#define PR_POLL_HUP     POLLHUP     /* only in out_flags */

#else  

#define PR_POLL_READ    0x1
#define PR_POLL_WRITE   0x2
#define PR_POLL_EXCEPT  0x4
#define PR_POLL_ERR     0x8         /* only in out_flags */
#define PR_POLL_NVAL    0x10        /* only in out_flags when fd is bad */
#define PR_POLL_HUP     0x20        /* only in out_flags */

#endif  









































NSPR_API(PRInt32) PR_Poll(
    PRPollDesc *pds, PRIntn npds, PRIntervalTime timeout);










































NSPR_API(PRFileDesc *) PR_NewPollableEvent(void);

NSPR_API(PRStatus) PR_DestroyPollableEvent(PRFileDesc *event);

NSPR_API(PRStatus) PR_SetPollableEvent(PRFileDesc *event);

NSPR_API(PRStatus) PR_WaitForPollableEvent(PRFileDesc *event);

PR_END_EXTERN_C

#endif 
