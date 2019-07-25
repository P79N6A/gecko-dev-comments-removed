








































#ifndef xptiprivate_h___
#define xptiprivate_h___

#include "nscore.h"
#include NEW_H
#include "nsISupports.h"



#include "xpt_struct.h"
#include "xpt_xdr.h"

#include "nsIInterfaceInfo.h"
#include "nsIInterfaceInfoManager.h"
#include "xptinfo.h"

#include "nsIServiceManager.h"
#include "nsILocalFile.h"
#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsAppDirectoryServiceDefs.h"
#include "nsIWeakReference.h"

#include "mozilla/ReentrantMonitor.h"
#include "mozilla/Mutex.h"

#include "nsCRT.h"
#include "nsMemory.h"

#include "nsCOMArray.h"
#include "nsQuickSort.h"

#include "nsXPIDLString.h"

#include "nsIInputStream.h"

#include "nsHashKeys.h"
#include "nsDataHashtable.h"
#include "plstr.h"
#include "prprf.h"
#include "prio.h"
#include "prtime.h"
#include "prenv.h"

#include <stdio.h>
#include <stdarg.h>



#if 0 && defined(DEBUG_jband)
#define LOG_RESOLVE(x) printf x
#define LOG_LOAD(x)    printf x
#define LOG_AUTOREG(x) do{printf x; xptiInterfaceInfoManager::WriteToLog x;}while(0)
#else
#define LOG_RESOLVE(x) ((void)0)
#define LOG_LOAD(x)    ((void)0)
#define LOG_AUTOREG(x) ((void)0)
#endif

#if 1 && defined(DEBUG_jband)
#define SHOW_INFO_COUNT_STATS
#endif



class xptiInterfaceInfo;
class xptiInterfaceInfoManager;
class xptiInterfaceEntry;
class xptiTypelibGuts;
class xptiWorkingSet;

extern XPTArena* gXPTIStructArena;









class xptiTypelibGuts
{
public:
    static xptiTypelibGuts* Create(XPTHeader* aHeader);

    XPTHeader*          GetHeader()           {return mHeader;}
    PRUint16            GetEntryCount() const {return mHeader->num_interfaces;}
    
    void                SetEntryAt(PRUint16 i, xptiInterfaceEntry* ptr)
    {
        NS_ASSERTION(mHeader,"bad state!");
        NS_ASSERTION(i < GetEntryCount(),"bad param!");
        mEntryArray[i] = ptr;
    }        

    xptiInterfaceEntry* GetEntryAt(PRUint16 i);

private:
    xptiTypelibGuts(XPTHeader* aHeader)
        : mHeader(aHeader)
    { }
    ~xptiTypelibGuts();

private:
    XPTHeader*           mHeader;        
    xptiInterfaceEntry*  mEntryArray[1]; 
};



class xptiWorkingSet
{
public:
    xptiWorkingSet();
    ~xptiWorkingSet();
    
    PRBool IsValid() const;

    void InvalidateInterfaceInfos();
    void ClearHashTables();

    

    enum {NOT_FOUND = 0xffffffff};

    

    PRUint32 GetDirectoryCount();
    nsresult GetCloneOfDirectoryAt(PRUint32 i, nsILocalFile** dir);
    nsresult GetDirectoryAt(PRUint32 i, nsILocalFile** dir);
    PRBool   FindDirectory(nsILocalFile* dir, PRUint32* index);
    PRBool   FindDirectoryOfFile(nsILocalFile* file, PRUint32* index);
    PRBool   DirectoryAtMatchesPersistentDescriptor(PRUint32 i, const char* desc);

private:
    PRUint32        mFileCount;
    PRUint32        mMaxFileCount;

public:
    
    
    
    
    
    mozilla::ReentrantMonitor mTableReentrantMonitor;
    nsDataHashtable<nsIDHashKey, xptiInterfaceEntry*> mIIDTable;
    nsDataHashtable<nsDepCharHashKey, xptiInterfaceEntry*> mNameTable;
};






class xptiInfoFlags
{
    enum {STATE_MASK = 3};
public:
    xptiInfoFlags(PRUint8 n) : mData(n) {}
    xptiInfoFlags(const xptiInfoFlags& r) : mData(r.mData) {}

    static PRUint8 GetStateMask()
        {return PRUint8(STATE_MASK);}
    
    void Clear()
        {mData = 0;}

    PRUint8 GetData() const
        {return mData;}

    PRUint8 GetState() const 
        {return mData & GetStateMask();}

    void SetState(PRUint8 state) 
        {mData &= ~GetStateMask(); mData |= state;}                                   

    void SetFlagBit(PRUint8 flag, PRBool on) 
        {if(on)
            mData |= ~GetStateMask() & flag;
         else
            mData &= GetStateMask() | ~flag;}

    PRBool GetFlagBit(PRUint8 flag) const 
        {return (mData & flag) ? PR_TRUE : PR_FALSE;}

private:
    PRUint8 mData;    
};







class xptiInterfaceEntry
{
public:
    static xptiInterfaceEntry* Create(const char* name,
                                      const nsID& iid,
                                      XPTInterfaceDescriptor* aDescriptor,
                                      xptiTypelibGuts* aTypelib);

    enum {
        PARTIALLY_RESOLVED    = 1,
        FULLY_RESOLVED        = 2,
        RESOLVE_FAILED        = 3
    };
    
    
    enum {SCRIPTABLE = 4};

    PRUint8 GetResolveState() const {return mFlags.GetState();}
    
    PRBool IsFullyResolved() const 
        {return GetResolveState() == (PRUint8) FULLY_RESOLVED;}

    void   SetScriptableFlag(PRBool on)
                {mFlags.SetFlagBit(PRUint8(SCRIPTABLE),on);}
    PRBool GetScriptableFlag() const
                {return mFlags.GetFlagBit(PRUint8(SCRIPTABLE));}

    const nsID* GetTheIID()  const {return &mIID;}
    const char* GetTheName() const {return mName;}

    PRBool EnsureResolved()
        {return IsFullyResolved() ? PR_TRUE : Resolve();}

    nsresult GetInterfaceInfo(xptiInterfaceInfo** info);
    PRBool   InterfaceInfoEquals(const xptiInterfaceInfo* info) const 
        {return info == mInfo;}
    
    void     LockedInvalidateInterfaceInfo();
    void     LockedInterfaceInfoDeathNotification() {mInfo = nsnull;}

    xptiInterfaceEntry* Parent() const {
        NS_ASSERTION(IsFullyResolved(), "Parent() called while not resolved?");
        return mParent;
    }

    const nsID& IID() const { return mIID; }

    
    

    nsresult GetName(char * *aName);
    nsresult GetIID(nsIID * *aIID);
    nsresult IsScriptable(PRBool *_retval);
    
    
    nsresult GetMethodCount(PRUint16 *aMethodCount);
    nsresult GetConstantCount(PRUint16 *aConstantCount);
    nsresult GetMethodInfo(PRUint16 index, const nsXPTMethodInfo * *info);
    nsresult GetMethodInfoForName(const char *methodName, PRUint16 *index, const nsXPTMethodInfo * *info);
    nsresult GetConstant(PRUint16 index, const nsXPTConstant * *constant);
    nsresult GetInfoForParam(PRUint16 methodIndex, const nsXPTParamInfo * param, nsIInterfaceInfo **_retval);
    nsresult GetIIDForParam(PRUint16 methodIndex, const nsXPTParamInfo * param, nsIID * *_retval);
    nsresult GetTypeForParam(PRUint16 methodIndex, const nsXPTParamInfo * param, PRUint16 dimension, nsXPTType *_retval);
    nsresult GetSizeIsArgNumberForParam(PRUint16 methodIndex, const nsXPTParamInfo * param, PRUint16 dimension, PRUint8 *_retval);
    nsresult GetLengthIsArgNumberForParam(PRUint16 methodIndex, const nsXPTParamInfo * param, PRUint16 dimension, PRUint8 *_retval);
    nsresult GetInterfaceIsArgNumberForParam(PRUint16 methodIndex, const nsXPTParamInfo * param, PRUint8 *_retval);
    nsresult IsIID(const nsIID * IID, PRBool *_retval);
    nsresult GetNameShared(const char **name);
    nsresult GetIIDShared(const nsIID * *iid);
    nsresult IsFunction(PRBool *_retval);
    nsresult HasAncestor(const nsIID * iid, PRBool *_retval);
    nsresult GetIIDForParamNoAlloc(PRUint16 methodIndex, const nsXPTParamInfo * param, nsIID *iid);

private:
    xptiInterfaceEntry(const char* name,
                       size_t nameLength,
                       const nsID& iid,
                       XPTInterfaceDescriptor* aDescriptor,
                       xptiTypelibGuts* aTypelib);
    ~xptiInterfaceEntry();

    void SetResolvedState(int state) 
        {mFlags.SetState(PRUint8(state));}

    PRBool Resolve();

    
    
    

    PRBool EnsureResolvedLocked()
        {return IsFullyResolved() ? PR_TRUE : ResolveLocked();}
    PRBool ResolveLocked();

    

    nsresult GetEntryForParam(PRUint16 methodIndex, 
                              const nsXPTParamInfo * param,
                              xptiInterfaceEntry** entry);

    nsresult GetTypeInArray(const nsXPTParamInfo* param,
                            PRUint16 dimension,
                            const XPTTypeDescriptor** type);

private:
    nsID                    mIID;
    XPTInterfaceDescriptor* mDescriptor;

    PRUint16 mMethodBaseIndex;
    PRUint16 mConstantBaseIndex;
    xptiTypelibGuts* mTypelib;

    xptiInterfaceEntry*     mParent;      

    xptiInterfaceInfo*      mInfo;        
    xptiInfoFlags           mFlags;
    char                    mName[1];     
};

class xptiInterfaceInfo : public nsIInterfaceInfo
{
public:
    NS_DECL_ISUPPORTS

    
    NS_IMETHOD GetName(char * *aName) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetName(aName); }
    NS_IMETHOD GetInterfaceIID(nsIID * *aIID) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetIID(aIID); }
    NS_IMETHOD IsScriptable(PRBool *_retval) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->IsScriptable(_retval); }
    
    NS_IMETHOD GetParent(nsIInterfaceInfo * *aParent) 
    {
        if(!EnsureResolved() || !EnsureParent())
            return NS_ERROR_UNEXPECTED;
        NS_IF_ADDREF(*aParent = mParent);
        return NS_OK;
    }
    NS_IMETHOD GetMethodCount(PRUint16 *aMethodCount) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetMethodCount(aMethodCount); }
    NS_IMETHOD GetConstantCount(PRUint16 *aConstantCount) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetConstantCount(aConstantCount); }
    NS_IMETHOD GetMethodInfo(PRUint16 index, const nsXPTMethodInfo * *info) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetMethodInfo(index, info); }
    NS_IMETHOD GetMethodInfoForName(const char *methodName, PRUint16 *index, const nsXPTMethodInfo * *info) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetMethodInfoForName(methodName, index, info); }
    NS_IMETHOD GetConstant(PRUint16 index, const nsXPTConstant * *constant) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetConstant(index, constant); }
    NS_IMETHOD GetInfoForParam(PRUint16 methodIndex, const nsXPTParamInfo * param, nsIInterfaceInfo **_retval) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetInfoForParam(methodIndex, param, _retval); }
    NS_IMETHOD GetIIDForParam(PRUint16 methodIndex, const nsXPTParamInfo * param, nsIID * *_retval) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetIIDForParam(methodIndex, param, _retval); }
    NS_IMETHOD GetTypeForParam(PRUint16 methodIndex, const nsXPTParamInfo * param, PRUint16 dimension, nsXPTType *_retval) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetTypeForParam(methodIndex, param, dimension, _retval); }
    NS_IMETHOD GetSizeIsArgNumberForParam(PRUint16 methodIndex, const nsXPTParamInfo * param, PRUint16 dimension, PRUint8 *_retval) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetSizeIsArgNumberForParam(methodIndex, param, dimension, _retval); }
    NS_IMETHOD GetLengthIsArgNumberForParam(PRUint16 methodIndex, const nsXPTParamInfo * param, PRUint16 dimension, PRUint8 *_retval) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetLengthIsArgNumberForParam(methodIndex, param, dimension, _retval); }
    NS_IMETHOD GetInterfaceIsArgNumberForParam(PRUint16 methodIndex, const nsXPTParamInfo * param, PRUint8 *_retval) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetInterfaceIsArgNumberForParam(methodIndex, param, _retval); }
    NS_IMETHOD IsIID(const nsIID * IID, PRBool *_retval) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->IsIID(IID, _retval); }
    NS_IMETHOD GetNameShared(const char **name) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetNameShared(name); }
    NS_IMETHOD GetIIDShared(const nsIID * *iid) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetIIDShared(iid); }
    NS_IMETHOD IsFunction(PRBool *_retval) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->IsFunction(_retval); }
    NS_IMETHOD HasAncestor(const nsIID * iid, PRBool *_retval) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->HasAncestor(iid, _retval); }
    NS_IMETHOD GetIIDForParamNoAlloc(PRUint16 methodIndex, const nsXPTParamInfo * param, nsIID *iid) { return !mEntry ? NS_ERROR_UNEXPECTED : mEntry->GetIIDForParamNoAlloc(methodIndex, param, iid); }

public:
    xptiInterfaceInfo(xptiInterfaceEntry* entry);

    void Invalidate() 
        {NS_IF_RELEASE(mParent); mEntry = nsnull;}

private:

    ~xptiInterfaceInfo();

    
    PRBool EnsureParent()
    {
        NS_ASSERTION(mEntry && mEntry->IsFullyResolved(), "bad EnsureParent call");
        return mParent || !mEntry->Parent() || BuildParent();
    }
    
    PRBool EnsureResolved()
    {
        return mEntry && mEntry->EnsureResolved();
    }

    PRBool BuildParent();

    xptiInterfaceInfo();  

private:
    xptiInterfaceEntry* mEntry;
    xptiInterfaceInfo*  mParent;
};



class xptiInterfaceInfoManager 
    : public nsIInterfaceInfoSuperManager
{
    NS_DECL_ISUPPORTS
    NS_DECL_NSIINTERFACEINFOMANAGER
    NS_DECL_NSIINTERFACEINFOSUPERMANAGER

    typedef mozilla::ReentrantMonitor ReentrantMonitor;
    typedef mozilla::Mutex Mutex;

public:
    
    static xptiInterfaceInfoManager* GetSingleton();
    static void FreeInterfaceInfoManager();

    void RegisterFile(nsILocalFile* aFile);
    void RegisterInputStream(nsIInputStream* aStream);

    xptiWorkingSet*  GetWorkingSet() {return &mWorkingSet;}

    static Mutex& GetResolveLock(xptiInterfaceInfoManager* self = nsnull) 
    {
        self = self ? self : GetSingleton();
        return self->mResolveLock;
    }

    xptiInterfaceEntry* GetInterfaceEntryForIID(const nsIID *iid);

private:
    xptiInterfaceInfoManager();
    ~xptiInterfaceInfoManager();

    void RegisterXPTHeader(XPTHeader* aHeader);
                          
    XPTHeader* ReadXPTFile(nsILocalFile* aFile);
    XPTHeader* ReadXPTFileFromInputStream(nsIInputStream *stream);

    
    void VerifyAndAddEntryIfNew(XPTInterfaceDirectoryEntry* iface,
                                PRUint16 idx,
                                xptiTypelibGuts* typelib);

private:
    xptiWorkingSet               mWorkingSet;
    Mutex                        mResolveLock;
    Mutex                        mAdditionalManagersLock;
    nsCOMArray<nsISupports>      mAdditionalManagers;
};

#endif 
