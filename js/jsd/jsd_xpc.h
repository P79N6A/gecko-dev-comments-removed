





#ifndef JSDSERVICE_H___
#define JSDSERVICE_H___

#include "jsdIDebuggerService.h"
#include "jsdebug.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nspr.h"
#include "nsCycleCollectionParticipant.h"
#include "mozilla/Attributes.h"





struct LiveEphemeral {
    
    PRCList                  links;
    jsdIEphemeral           *value;
    void                    *key;
};

struct PCMapEntry {
    uint32_t pc, line;
};
    




class jsdObject MOZ_FINAL : public jsdIObject
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDIOBJECT

    
    jsdObject (JSDContext *aCx, JSDObject *aObject) :
        mCx(aCx), mObject(aObject)
    {
    }

    static jsdIObject *FromPtr (JSDContext *aCx,
                                JSDObject *aObject)
    {
        if (!aObject)
            return nullptr;
        
        jsdIObject *rv = new jsdObject (aCx, aObject);
        NS_IF_ADDREF(rv);
        return rv;
    }

  private:
    jsdObject(); 
    jsdObject(const jsdObject&); 

    JSDContext *mCx;
    JSDObject *mObject;
};


class jsdProperty : public jsdIProperty
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDIPROPERTY
    NS_DECL_JSDIEPHEMERAL
    
    jsdProperty (JSDContext *aCx, JSDProperty *aProperty);
    virtual ~jsdProperty ();
    
    static jsdIProperty *FromPtr (JSDContext *aCx,
                                  JSDProperty *aProperty)
    {
        if (!aProperty)
            return nullptr;
        
        jsdIProperty *rv = new jsdProperty (aCx, aProperty);
        NS_IF_ADDREF(rv);
        return rv;
    }

    static void InvalidateAll();

  private:
    jsdProperty(); 
    jsdProperty(const jsdProperty&); 

    bool           mValid;
    LiveEphemeral  mLiveListEntry;
    JSDContext    *mCx;
    JSDProperty   *mProperty;
};

class jsdScript : public jsdIScript
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDISCRIPT
    NS_DECL_JSDIEPHEMERAL

    
    jsdScript (JSDContext *aCx, JSDScript *aScript);
    virtual ~jsdScript();
    
    static jsdIScript *FromPtr (JSDContext *aCx, JSDScript *aScript)
    {
        if (!aScript)
            return nullptr;

        void *data = JSD_GetScriptPrivate (aScript);
        jsdIScript *rv;
        
        if (data) {
            rv = static_cast<jsdIScript *>(data);
        } else {
            rv = new jsdScript (aCx, aScript);
            NS_IF_ADDREF(rv);  

            JSD_SetScriptPrivate (aScript, static_cast<void *>(rv));
        }
        
        NS_IF_ADDREF(rv); 
        return rv;
    }

    static void InvalidateAll();

  private:
    static uint32_t LastTag;
    
    jsdScript(); 
    jsdScript (const jsdScript&); 
    PCMapEntry* CreatePPLineMap();
    uint32_t    PPPcToLine(uint32_t aPC);
    uint32_t    PPLineToPc(uint32_t aLine);
    
    bool        mValid;
    uint32_t    mTag;
    JSDContext *mCx;
    JSDScript  *mScript;
    nsCString  *mFileName;
    nsCString  *mFunctionName;
    uint32_t    mBaseLineNumber, mLineExtent;
    PCMapEntry *mPPLineMap;
    uint32_t    mPCMapSize;
    uintptr_t   mFirstPC;
};

uint32_t jsdScript::LastTag = 0;

class jsdContext : public jsdIContext
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDICONTEXT
    NS_DECL_JSDIEPHEMERAL

    jsdContext (JSDContext *aJSDCx, JSContext *aJSCx, nsISupports *aISCx);
    virtual ~jsdContext();

    static void InvalidateAll();
    static jsdIContext *FromPtr (JSDContext *aJSDCx, JSContext *aJSCx);
  private:
    static uint32_t LastTag;

    jsdContext (); 
    jsdContext (const jsdContext&); 

    bool                   mValid;
    LiveEphemeral          mLiveListEntry;
    uint32_t               mTag;
    JSDContext            *mJSDCx;
    JSContext             *mJSCx;
    nsCOMPtr<nsISupports>  mISCx;
};

uint32_t jsdContext::LastTag = 0;

class jsdStackFrame : public jsdIStackFrame
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDISTACKFRAME
    NS_DECL_JSDIEPHEMERAL

    
    jsdStackFrame (JSDContext *aCx, JSDThreadState *aThreadState,
                   JSDStackFrameInfo *aStackFrameInfo);
    virtual ~jsdStackFrame();

    static void InvalidateAll();
    static jsdIStackFrame* FromPtr (JSDContext *aCx,
                                    JSDThreadState *aThreadState,
                                    JSDStackFrameInfo *aStackFrameInfo);

  private:
    jsdStackFrame(); 
    jsdStackFrame(const jsdStackFrame&); 

    bool               mValid;
    LiveEphemeral      mLiveListEntry;
    JSDContext        *mCx;
    JSDThreadState    *mThreadState;
    JSDStackFrameInfo *mStackFrameInfo;
};

class jsdValue : public jsdIValue
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDIVALUE
    NS_DECL_JSDIEPHEMERAL

    
    jsdValue (JSDContext *aCx, JSDValue *aValue);
    virtual ~jsdValue();

    static jsdIValue *FromPtr (JSDContext *aCx, JSDValue *aValue);    
    static void InvalidateAll();
    
  private:
    jsdValue(); 
    jsdValue (const jsdScript&); 
    
    bool           mValid;
    LiveEphemeral  mLiveListEntry;
    JSDContext    *mCx;
    JSDValue      *mValue;
};





class jsdService : public jsdIDebuggerService
{
  public:
    NS_DECL_CYCLE_COLLECTING_ISUPPORTS
    NS_DECL_JSDIDEBUGGERSERVICE

    NS_DECL_CYCLE_COLLECTION_CLASS(jsdService)

    jsdService() : mOn(false), mPauseLevel(0),
                   mNestedLoopLevel(0), mCx(0), mRuntime(0), mErrorHook(0),
                   mBreakpointHook(0), mDebugHook(0), mDebuggerHook(0),
                   mInterruptHook(0), mScriptHook(0), mThrowHook(0),
                   mTopLevelHook(0), mFunctionHook(0)
    {
    }

    virtual ~jsdService();
    
    static jsdService *GetService ();

    bool CheckInterruptHook() { return !!mInterruptHook; }
    
    nsresult DoPause(uint32_t *_rval, bool internalCall);
    nsresult DoUnPause(uint32_t *_rval, bool internalCall);

  private:
    bool        mOn;
    uint32_t    mPauseLevel;
    uint32_t    mNestedLoopLevel;
    JSDContext *mCx;
    JSRuntime  *mRuntime;

    nsCOMPtr<jsdIErrorHook>     mErrorHook;
    nsCOMPtr<jsdIExecutionHook> mBreakpointHook;
    nsCOMPtr<jsdIExecutionHook> mDebugHook;
    nsCOMPtr<jsdIExecutionHook> mDebuggerHook;
    nsCOMPtr<jsdIExecutionHook> mInterruptHook;
    nsCOMPtr<jsdIScriptHook>    mScriptHook;
    nsCOMPtr<jsdIExecutionHook> mThrowHook;
    nsCOMPtr<jsdICallHook>      mTopLevelHook;
    nsCOMPtr<jsdICallHook>      mFunctionHook;
    nsCOMPtr<jsdIActivationCallback> mActivationCallback;
};

#endif 




#if 0

class jsdContext : public jsdIContext
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDICONTEXT

    
    jsdContext (JSDContext *aCx) : mCx(aCx)
    {
        printf ("++++++ jsdContext\n");
    }

    static jsdIContext *FromPtr (JSDContext *aCx)
    {
        if (!aCx)
            return nullptr;
        
        void *data = JSD_GetContextPrivate (aCx);
        jsdIContext *rv;
        
        if (data) {
            rv = static_cast<jsdIContext *>(data);
        } else {
            rv = new jsdContext (aCx);
            NS_IF_ADDREF(rv);  
            JSD_SetContextPrivate (aCx, static_cast<void *>(rv));
        }
        
        NS_IF_ADDREF(rv); 
        return rv;
    }

    virtual ~jsdContext() { printf ("------ ~jsdContext\n"); }
  private:            
    jsdContext(); 
    jsdContext(const jsdContext&); 
    
    JSDContext *mCx;
};

class jsdThreadState : public jsdIThreadState
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDITHREADSTATE

    
    jsdThreadState (JSDContext *aCx, JSDThreadState *aThreadState) :
        mCx(aCx), mThreadState(aThreadState)
    {
    }

    






    static jsdIThreadState *FromPtr (JSDContext *aCx,
                                     JSDThreadState *aThreadState)
    {
        if (!aThreadState)
            return nullptr;
        
        jsdIThreadState *rv = new jsdThreadState (aCx, aThreadState);
        NS_IF_ADDREF(rv);
        return rv;
    }

  private:
    jsdThreadState(); 
    jsdThreadState(const jsdThreadState&); 

    JSDContext     *mCx;
    JSDThreadState *mThreadState;
};

#endif
