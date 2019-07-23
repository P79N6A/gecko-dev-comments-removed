






































#ifndef JSDSERVICE_H___
#define JSDSERVICE_H___

#include "jsdIDebuggerService.h"
#include "jsdebug.h"
#include "nsString.h"
#include "nsCOMPtr.h"
#include "nspr.h"





struct LiveEphemeral {
    
    PRCList                  links;
    jsdIEphemeral           *value;
    void                    *key;
};

struct PCMapEntry {
    PRUint32 pc, line;
};
    




class jsdObject : public jsdIObject
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
            return nsnull;
        
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
            return nsnull;
        
        jsdIProperty *rv = new jsdProperty (aCx, aProperty);
        NS_IF_ADDREF(rv);
        return rv;
    }

    static void InvalidateAll();

  private:
    jsdProperty(); 
    jsdProperty(const jsdProperty&); 

    PRBool         mValid;
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
            return nsnull;

        void *data = JSD_GetScriptPrivate (aScript);
        jsdIScript *rv;
        
        if (data) {
            rv = NS_STATIC_CAST(jsdIScript *, data);
        } else {
            rv = new jsdScript (aCx, aScript);
            NS_IF_ADDREF(rv);  

            JSD_SetScriptPrivate (aScript, NS_STATIC_CAST(void *, rv));
        }
        
        NS_IF_ADDREF(rv); 
        return rv;
    }

    static void InvalidateAll();

  private:
    static PRUint32 LastTag;
    
    jsdScript(); 
    jsdScript (const jsdScript&); 
    PCMapEntry* CreatePPLineMap();
    PRUint32    PPPcToLine(PRUint32 aPC);
    PRUint32    PPLineToPc(PRUint32 aLine);
    
    PRBool      mValid;
    PRUint32    mTag;
    JSDContext *mCx;
    JSDScript  *mScript;
    nsCString  *mFileName;
    nsCString  *mFunctionName;
    PRUint32    mBaseLineNumber, mLineExtent;
    PCMapEntry *mPPLineMap;
    PRUint32    mPCMapSize;
    jsuword     mFirstPC;
};

PRUint32 jsdScript::LastTag = 0;

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
    static PRUint32 LastTag;

    jsdContext (); 
    jsdContext (const jsdContext&); 

    PRBool                 mValid;
    LiveEphemeral          mLiveListEntry;
    PRUint32               mTag;
    JSDContext            *mJSDCx;
    JSContext             *mJSCx;
    nsCOMPtr<nsISupports>  mISCx;
};

PRUint32 jsdContext::LastTag = 0;

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

    PRBool             mValid;
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
    
    PRBool         mValid;
    LiveEphemeral  mLiveListEntry;
    JSDContext    *mCx;
    JSDValue      *mValue;
};





class jsdService : public jsdIDebuggerService
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_JSDIDEBUGGERSERVICE

    jsdService() : mInitAtStartup(triUnknown), mOn(PR_FALSE), mPauseLevel(0),
                   mNestedLoopLevel(0), mCx(0), mRuntime(0), mErrorHook(0),
                   mBreakpointHook(0), mDebugHook(0), mDebuggerHook(0),
                   mInterruptHook(0), mScriptHook(0), mThrowHook(0),
                   mTopLevelHook(0), mFunctionHook(0)
    {
    }

    virtual ~jsdService();
    
    static jsdService *GetService ();
    
  private:
    enum Tristate {
        triUnknown = 0U,
        triYes = 1U,
        triNo = 2U
    };
        
    Tristate    mInitAtStartup;
    PRBool      mOn;
    PRUint32    mPauseLevel;
    PRUint32    mNestedLoopLevel;
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
            return nsnull;
        
        void *data = JSD_GetContextPrivate (aCx);
        jsdIContext *rv;
        
        if (data) {
            rv = NS_STATIC_CAST(jsdIContext *, data);
        } else {
            rv = new jsdContext (aCx);
            NS_IF_ADDREF(rv);  
            JSD_SetContextPrivate (aCx, NS_STATIC_CAST(void *, rv));
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
            return nsnull;
        
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
