



#ifndef nsASocketHandler_h__
#define nsASocketHandler_h__




class nsASocketHandler : public nsISupports
{
public:
    nsASocketHandler()
        : mCondition(NS_OK)
        , mPollFlags(0)
        , mPollTimeout(UINT16_MAX)
        , mIsPrivate(false)
        {}

    
    
    
    
    
    nsresult mCondition;

    
    
    
    
    
    uint16_t mPollFlags;

    
    
    
    
    
    
    
    
    
    uint16_t mPollTimeout;

    bool mIsPrivate;

    
    
    
    
    
    
    
    
    
    virtual void OnSocketReady(PRFileDesc *fd, int16_t outFlags) = 0;

    
    
    
    
    
    
    virtual void OnSocketDetached(PRFileDesc *fd) = 0;

    
    
    
    
    
    virtual void IsLocal(bool *aIsLocal) = 0;

    
    
    
    
    
    virtual void KeepWhenOffline(bool *aKeepWhenOffline)
    {
        *aKeepWhenOffline = false;
    }

    
    
    
    virtual uint64_t ByteCountSent() = 0;
    virtual uint64_t ByteCountReceived() = 0;
};

#endif 
