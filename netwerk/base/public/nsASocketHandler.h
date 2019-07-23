





































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

    
    
    
    
    
    PRUint16 mPollFlags;

    
    
    
    
    
    
    
    
    
    PRUint16 mPollTimeout;

    
    
    
    
    
    
    
    
    
    virtual void OnSocketReady(PRFileDesc *fd, PRInt16 outFlags) = 0;

    
    
    
    
    
    
    virtual void OnSocketDetached(PRFileDesc *fd) = 0;
};

#endif 
