









#ifndef WEBRTC_SYSTEM_WRAPPERS_INTERFACE_RW_LOCK_WRAPPER_H_
#define WEBRTC_SYSTEM_WRAPPERS_INTERFACE_RW_LOCK_WRAPPER_H_





namespace webrtc {
class RWLockWrapper
{
public:
    static RWLockWrapper* CreateRWLock();
    virtual ~RWLockWrapper();

    virtual void AcquireLockExclusive() = 0;
    virtual void ReleaseLockExclusive() = 0;

    virtual void AcquireLockShared() = 0;
    virtual void ReleaseLockShared() = 0;

protected:
    virtual int Init() = 0;
};



class ReadLockScoped
{
public:
    ReadLockScoped(RWLockWrapper& rwLock)
        :
        _rwLock(rwLock)
    {
        _rwLock.AcquireLockShared();
    }

    ~ReadLockScoped()
    {
        _rwLock.ReleaseLockShared();
    }

private:
    RWLockWrapper& _rwLock;
};

class WriteLockScoped
{
public:
    WriteLockScoped(RWLockWrapper& rwLock)
        :
        _rwLock(rwLock)
    {
        _rwLock.AcquireLockExclusive();
    }

    ~WriteLockScoped()
    {
        _rwLock.ReleaseLockExclusive();
    }

private:
    RWLockWrapper& _rwLock;
};
} 

#endif 
