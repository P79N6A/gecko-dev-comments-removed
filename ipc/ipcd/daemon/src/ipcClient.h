




































#ifndef ipcClientUnix_h__
#define ipcClientUnix_h__

#include "prio.h"
#include "ipcMessageQ.h"
#include "ipcStringList.h"
#include "ipcIDList.h"

#ifdef XP_WIN
#include <windows.h>
#endif









class ipcClient
{
public:
    void Init();
    void Finalize();

    PRUint32 ID() const { return mID; }

    void   AddName(const char *name);
    void   DelName(const char *name);
    PRBool HasName(const char *name) const { return mNames.Find(name) != NULL; }

    void   AddTarget(const nsID &target);
    void   DelTarget(const nsID &target);
    PRBool HasTarget(const nsID &target) const { return mTargets.Find(target) != NULL; }

    
    const ipcStringNode *Names()   const { return mNames.First(); }
    const ipcIDNode     *Targets() const { return mTargets.First(); }

    
    const char *PrimaryName() const { return mNames.First() ? mNames.First()->Value() : NULL; }

    void   SetExpectsSyncReply(PRBool val) { mExpectsSyncReply = val; }
    PRBool GetExpectsSyncReply() const     { return mExpectsSyncReply; }

#ifdef XP_WIN
    PRUint32 PID() const { return mPID; }
    void SetPID(PRUint32 pid) { mPID = pid; }

    HWND Hwnd() const { return mHwnd; }
    void SetHwnd(HWND hwnd) { mHwnd = hwnd; }
#endif

#if defined(XP_UNIX) || defined(XP_OS2)
    
    
    
    
    
    
    
    
    
    
    
    
    int Process(PRFileDesc *sockFD, int pollFlags);

    
    
    
    
    void EnqueueOutboundMsg(ipcMessage *msg) { mOutMsgQ.Append(msg); }
#endif

private:
    static PRUint32 gLastID;

    PRUint32      mID;
    ipcStringList mNames;
    ipcIDList     mTargets;
    PRBool        mExpectsSyncReply;

#ifdef XP_WIN
    
    
    
    PRUint32      mPID;
    
    
    HWND          mHwnd;
#endif

#if defined(XP_UNIX) || defined(XP_OS2)
    ipcMessage    mInMsg;    
    ipcMessageQ   mOutMsgQ;  

    
    PRUint32      mSendOffset;

    
    int WriteMsgs(PRFileDesc *fd);
#endif
};

#endif 
