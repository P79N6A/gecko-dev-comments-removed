











































#if defined(_RCIO_H)
#else
#define _RCIO_H

#include "rcbase.h"
#include "rcnetdb.h"
#include "rcinrval.h"

#include "prio.h"

class RCFileInfo;

class PR_IMPLEMENT(RCIO): public RCBase
{
public:
    typedef enum {
        open = PR_TRANSMITFILE_KEEP_OPEN,   

        close = PR_TRANSMITFILE_CLOSE_SOCKET

    } FileDisposition;

    typedef enum {
        set = PR_SEEK_SET,                  
        current = PR_SEEK_CUR,              
        end = PR_SEEK_END                   
    } Whence;

    typedef enum {
        recv = PR_SHUTDOWN_RCV,             
        send = PR_SHUTDOWN_SEND,            
        both = PR_SHUTDOWN_BOTH             
    } ShutdownHow;

public:
    virtual ~RCIO();

    virtual RCIO*       Accept(RCNetAddr* addr, const RCInterval& timeout) = 0;
    virtual PRInt32     AcceptRead(
                            RCIO **nd, RCNetAddr **raddr, void *buf,
                            PRSize amount, const RCInterval& timeout) = 0;
    virtual PRInt64     Available() = 0;
    virtual PRStatus    Bind(const RCNetAddr& addr) = 0;
    virtual PRStatus    Close() = 0;
    virtual PRStatus    Connect(
                            const RCNetAddr& addr,
                            const RCInterval& timeout) = 0;
    virtual PRStatus    FileInfo(RCFileInfo *info) const = 0;
    virtual PRStatus    Fsync() = 0;
    virtual PRStatus    GetLocalName(RCNetAddr *addr) const = 0;
    virtual PRStatus    GetPeerName(RCNetAddr *addr) const = 0;
    virtual PRStatus    GetSocketOption(PRSocketOptionData *data) const = 0;
    virtual PRStatus    Listen(PRIntn backlog) = 0;
    virtual PRStatus    Open(const char *name, PRIntn flags, PRIntn mode) = 0;
    virtual PRInt16     Poll(PRInt16 in_flags, PRInt16 *out_flags) = 0;
    virtual PRInt32     Read(void *buf, PRSize amount) = 0;
    virtual PRInt32     Recv(
                            void *buf, PRSize amount, PRIntn flags,
                            const RCInterval& timeout) = 0;
    virtual PRInt32     Recvfrom(
                            void *buf, PRSize amount, PRIntn flags,
                            RCNetAddr* addr, const RCInterval& timeout) = 0;
    virtual PRInt64     Seek(PRInt64 offset, Whence how) = 0;
    virtual PRInt32     Send(
                            const void *buf, PRSize amount, PRIntn flags,
                            const RCInterval& timeout) = 0;
    virtual PRInt32     Sendto(
                            const void *buf, PRSize amount, PRIntn flags,
                            const RCNetAddr& addr,
                            const RCInterval& timeout) = 0;
    virtual PRStatus    SetSocketOption(const PRSocketOptionData *data) = 0;
    virtual PRStatus    Shutdown(ShutdownHow how) = 0;
    virtual PRInt32     TransmitFile(
                            RCIO *source, const void *headers,
                            PRSize hlen, RCIO::FileDisposition flags,
                            const RCInterval& timeout) = 0;
    virtual PRInt32     Write(const void *buf, PRSize amount) = 0;
    virtual PRInt32     Writev(
                            const PRIOVec *iov, PRSize size,
                            const RCInterval& timeout) = 0;

protected:
    typedef enum {
        file = PR_DESC_FILE,
        tcp = PR_DESC_SOCKET_TCP,
        udp = PR_DESC_SOCKET_UDP,
        layered = PR_DESC_LAYERED} RCIOType;

    RCIO(RCIOType);

    PRFileDesc *fd;  

private:
    
    RCIO();
    RCIO(const RCIO&);

};  

#endif 




