








































#if defined(_RCFILEIO_H)
#else
#define _RCFILEIO_H

#include "rcio.h"
#include "rctime.h"









class RCFileInfo;

class PR_IMPLEMENT(RCFileIO): public RCIO
{
public:
    RCFileIO();
    virtual ~RCFileIO();

    virtual PRInt64     Available();
    virtual PRStatus    Close();
    static  PRStatus    Delete(const char *name);
    virtual PRStatus    FileInfo(RCFileInfo* info) const;
    static  PRStatus    FileInfo(const char *name, RCFileInfo* info);
    virtual PRStatus    Fsync();
    virtual PRStatus    Open(const char *name, PRIntn flags, PRIntn mode);
    virtual PRInt32     Read(void *buf, PRSize amount);
    virtual PRInt64     Seek(PRInt64 offset, RCIO::Whence how);
    virtual PRInt32     Write(const void *buf, PRSize amount);
    virtual PRInt32     Writev(
                            const PRIOVec *iov, PRSize size,
                            const RCInterval& timeout);

private:

    
    RCFileIO(const RCFileIO&);
    void operator=(const RCFileIO&);

    RCIO*       Accept(RCNetAddr* addr, const RCInterval& timeout);
    PRInt32     AcceptRead(
                    RCIO **newfd, RCNetAddr **address, void *buffer,
                    PRSize amount, const RCInterval& timeout);
    PRStatus    Bind(const RCNetAddr& addr);
    PRStatus    Connect(const RCNetAddr& addr, const RCInterval& timeout);
    PRStatus    GetLocalName(RCNetAddr *addr) const;
    PRStatus    GetPeerName(RCNetAddr *addr) const;
    PRStatus    GetSocketOption(PRSocketOptionData *data) const;
    PRStatus    Listen(PRIntn backlog);
    PRInt16     Poll(PRInt16 in_flags, PRInt16 *out_flags);
    PRInt32     Recv(
                    void *buf, PRSize amount, PRIntn flags,
                    const RCInterval& timeout);
    PRInt32     Recvfrom(
                    void *buf, PRSize amount, PRIntn flags,
                    RCNetAddr* addr, const RCInterval& timeout);
    PRInt32     Send(
                    const void *buf, PRSize amount, PRIntn flags,
                    const RCInterval& timeout);
    PRInt32     Sendto(
                    const void *buf, PRSize amount, PRIntn flags,
                    const RCNetAddr& addr,
                    const RCInterval& timeout);
    PRStatus    SetSocketOption(const PRSocketOptionData *data);
    PRStatus    Shutdown(RCIO::ShutdownHow how);
    PRInt32     TransmitFile(
                    RCIO *source, const void *headers,
                    PRSize hlen, RCIO::FileDisposition flags,
                    const RCInterval& timeout);
public:

    



    typedef enum {
        input = PR_StandardInput,
        output = PR_StandardOutput,
        error = PR_StandardError
    } SpecialFile;

    static RCIO *GetSpecialFile(RCFileIO::SpecialFile special);

};  

class PR_IMPLEMENT(RCFileInfo): public RCBase
{
public:
    typedef enum {
        file = PR_FILE_FILE,
        directory = PR_FILE_DIRECTORY,
        other = PR_FILE_OTHER
    } FileType;

public:
    RCFileInfo();
    RCFileInfo(const RCFileInfo&);

    virtual ~RCFileInfo();

    PRInt64 Size() const;
    RCTime CreationTime() const;
    RCTime ModifyTime() const;
    RCFileInfo::FileType Type() const;

friend PRStatus RCFileIO::FileInfo(RCFileInfo*) const;
friend PRStatus RCFileIO::FileInfo(const char *name, RCFileInfo*);

private:
    PRFileInfo64 info;
};  

inline RCFileInfo::RCFileInfo(): RCBase() { }
inline PRInt64 RCFileInfo::Size() const { return info.size; }

#endif 
