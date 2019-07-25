



#ifndef nsASocketHandler_h__
#define nsASocketHandler_h__




class nsASocketHandler : public nsISupports
{
public:
    nsASocketHandler()
        : mCondition(NS_OK)
        , mPollFlags(0)
        , mPollTimeout(PR_UINT16_MAX)
        {}

    
    
    
    
    
    nsresult mCondition;

    
    
    
    
    
    uint16_t mPollFlags;

    
    
    
    
    
    
    
    
    
    uint16_t mPollTimeout;

    
    
    
    
    
    
    
    
    
    virtual void OnSocketReady(PRFileDesc *fd, int16_t outFlags) = 0;

    
    
    
    
    
    
    virtual void OnSocketDetached(PRFileDesc *fd) = 0;
};

#endif 
