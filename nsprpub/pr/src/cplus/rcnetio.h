








































#if defined(_RCNETIO_H)
#else
#define _RCNETIO_H

#include "rcbase.h"
#include "rcinrval.h"
#include "rcio.h"
#include "rcnetdb.h"

#include "prio.h"

class RCFileInfo;









class PR_IMPLEMENT(RCNetStreamIO): public RCIO
{

public:
    RCNetStreamIO();
    virtual ~RCNetStreamIO();

    virtual RCIO*       Accept(RCNetAddr* addr, const RCInterval& timeout);
    virtual PRInt32     AcceptRead(
                            RCIO **nd, RCNetAddr **raddr, void *buf,
                            PRSize amount, const RCInterval& timeout);
    virtual PRInt64     Available();
    virtual PRStatus    Bind(const RCNetAddr& addr);
    virtual PRStatus    Connect(
                            const RCNetAddr& addr, const RCInterval& timeout);
    virtual PRStatus    GetLocalName(RCNetAddr *addr) const;
    virtual PRStatus    GetPeerName(RCNetAddr *addr) const;
    virtual PRStatus    GetSocketOption(PRSocketOptionData *data) const;
    virtual PRStatus    Listen(PRIntn backlog);
    virtual PRInt16     Poll(PRInt16 in_flags, PRInt16 *out_flags);
    virtual PRInt32     Read(void *buf, PRSize amount);
    virtual PRInt32     Recv(
                            void *buf, PRSize amount, PRIntn flags,
                            const RCInterval& timeout);
    virtual PRInt32     Recvfrom(
                            void *buf, PRSize amount, PRIntn flags,
                            RCNetAddr* addr, const RCInterval& timeout);
    virtual PRInt32     Send(
                            const void *buf, PRSize amount, PRIntn flags,
                            const RCInterval& timeout);
    virtual PRInt32     Sendto(
                            const void *buf, PRSize amount, PRIntn flags,
                            const RCNetAddr& addr,
                            const RCInterval& timeout);
    virtual PRStatus    SetSocketOption(const PRSocketOptionData *data);
    virtual PRStatus    Shutdown(ShutdownHow how);
    virtual PRInt32     TransmitFile(
                            RCIO *source, const void *headers,
                            PRSize hlen, RCIO::FileDisposition flags,
                            const RCInterval& timeout);
    virtual PRInt32     Write(const void *buf, PRSize amount);
    virtual PRInt32     Writev(
                            const PRIOVec *iov, PRSize size,
                            const RCInterval& timeout);

private:
    
    RCNetStreamIO(const RCNetStreamIO&);

    PRStatus    Close();
    PRStatus    Open(const char *name, PRIntn flags, PRIntn mode);
    PRStatus    FileInfo(RCFileInfo *info) const;
    PRStatus    Fsync();
    PRInt64     Seek(PRInt64 offset, RCIO::Whence how);

public:
    RCNetStreamIO(PRIntn protocol);
};  

#endif 




