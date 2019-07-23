





































#ifndef __CBrowserShellMsgDefs__
#define __CBrowserShellMsgDefs__

#pragma once



#ifndef EMBED_MSG_BASE_ID
#define EMBED_MSG_BASE_ID 1000
#endif

enum {
    msg_OnNetStartChange      = EMBED_MSG_BASE_ID + 0,
    msg_OnNetStopChange 	  = EMBED_MSG_BASE_ID + 1,
    msg_OnProgressChange      = EMBED_MSG_BASE_ID + 2,
    msg_OnLocationChange      = EMBED_MSG_BASE_ID + 3,
    msg_OnStatusChange        = EMBED_MSG_BASE_ID + 4,
    msg_OnSecurityChange      = EMBED_MSG_BASE_ID + 5,
    
    msg_OnChromeStatusChange  = EMBED_MSG_BASE_ID + 6
};








struct MsgNetStartInfo
{
    MsgNetStartInfo(CBrowserShell* broadcaster) :
        mBroadcaster(broadcaster)
        { }
    
    CBrowserShell *mBroadcaster;      
};


struct MsgNetStopInfo
{
    MsgNetStopInfo(CBrowserShell* broadcaster) :
        mBroadcaster(broadcaster)
        { }
    
    CBrowserShell *mBroadcaster;      
};


struct MsgOnProgressChangeInfo
{
    MsgOnProgressChangeInfo(CBrowserShell* broadcaster, PRInt32 curProgress, PRInt32 maxProgress) :
        mBroadcaster(broadcaster), mCurProgress(curProgress), mMaxProgress(maxProgress)
        { }
    
    CBrowserShell *mBroadcaster;      
    PRInt32 mCurProgress, mMaxProgress;
};


struct MsgLocationChangeInfo
{
    MsgLocationChangeInfo(CBrowserShell* broadcaster,
                          const char* urlSpec) :
        mBroadcaster(broadcaster), mURLSpec(urlSpec)
        { }
    
    CBrowserShell *mBroadcaster;
    const char *mURLSpec;     
};


struct MsgStatusChangeInfo
{
    MsgStatusChangeInfo(CBrowserShell* broadcaster,
                        nsresult status, const PRUnichar *message) :
        mBroadcaster(broadcaster),
        mStatus(status), mMessage(message)
        { }
    
    CBrowserShell *mBroadcaster;
    nsresult mStatus;
    const PRUnichar *mMessage;     
};


struct MsgSecurityChangeInfo
{
    MsgSecurityChangeInfo(CBrowserShell* broadcaster,
                          PRUint32 state) :
        mBroadcaster(broadcaster),
        mState(state)
        { }
    
    CBrowserShell *mBroadcaster;
    PRUint32 mState;
}; 



struct MsgChromeStatusChangeInfo
{
    MsgChromeStatusChangeInfo(CBrowserShell* broadcaster,
                              PRUint32 statusType,
                              const PRUnichar* status) :
        mBroadcaster(broadcaster),
        mStatusType(statusType), mStatus(status)
        { }
                     
    CBrowserShell *mBroadcaster;
    PRUint32  mStatusType;
    const PRUnichar *mStatus;
};

#endif 
