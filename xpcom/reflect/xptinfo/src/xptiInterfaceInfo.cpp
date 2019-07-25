








































#include "xptiprivate.h"
#include "nsAtomicRefcnt.h"

using namespace mozilla;




#ifdef SHOW_INFO_COUNT_STATS
static int DEBUG_TotalInfos = 0;
static int DEBUG_CurrentInfos = 0;
static int DEBUG_MaxInfos = 0;
static int DEBUG_MonitorEntryCount = 0;

#define LOG_INFO_CREATE(t)                                                  \
    DEBUG_TotalInfos++;                                                     \
    DEBUG_CurrentInfos++;                                                   \
    if(DEBUG_MaxInfos < DEBUG_CurrentInfos)                                 \
        DEBUG_MaxInfos = DEBUG_CurrentInfos /* no ';' */

#define LOG_INFO_DESTROY(t)                                                 \
    DEBUG_CurrentInfos-- /* no ';' */

#define LOG_INFO_MONITOR_ENTRY                                              \
    DEBUG_MonitorEntryCount++ /* no ';' */

#else 

#define LOG_INFO_CREATE(t)     ((void)0)
#define LOG_INFO_DESTROY(t)    ((void)0)
#define LOG_INFO_MONITOR_ENTRY ((void)0)
#endif 

 xptiInterfaceEntry*
xptiInterfaceEntry::Create(const char* name, const nsID& iid,
                           XPTInterfaceDescriptor* aDescriptor,
                           xptiTypelibGuts* aTypelib)
{
    int namelen = strlen(name);
    return new (XPT_MALLOC(gXPTIStructArena,
                           sizeof(xptiInterfaceEntry) + namelen))
        xptiInterfaceEntry(name, namelen, iid, aDescriptor, aTypelib);
}

xptiInterfaceEntry::xptiInterfaceEntry(const char* name,
                                       size_t nameLength,
                                       const nsID& iid,
                                       XPTInterfaceDescriptor* aDescriptor,
                                       xptiTypelibGuts* aTypelib)
    : mIID(iid)
    , mDescriptor(aDescriptor)
    , mMethodBaseIndex(0)
    , mConstantBaseIndex(0)
    , mTypelib(aTypelib)
    , mParent(NULL)
    , mInfo(NULL)
    , mFlags(0)
{
    memcpy(mName, name, nameLength);
    SetResolvedState(PARTIALLY_RESOLVED);
}

PRBool 
xptiInterfaceEntry::Resolve()
{
    MutexAutoLock lock(xptiInterfaceInfoManager::GetResolveLock());
    return ResolveLocked();
}

PRBool 
xptiInterfaceEntry::ResolveLocked()
{
    int resolvedState = GetResolveState();

    if(resolvedState == FULLY_RESOLVED)
        return PR_TRUE;
    if(resolvedState == RESOLVE_FAILED)
        return PR_FALSE;

    NS_ASSERTION(GetResolveState() == PARTIALLY_RESOLVED, "bad state!");    

    
    

    PRUint16 parent_index = mDescriptor->parent_interface;

    if(parent_index)
    {
        xptiInterfaceEntry* parent = 
            mTypelib->GetEntryAt(parent_index - 1);
        
        if(!parent || !parent->EnsureResolvedLocked())
        {
            SetResolvedState(RESOLVE_FAILED);
            return PR_FALSE;
        }

        mParent = parent;

        mMethodBaseIndex =
            parent->mMethodBaseIndex + 
            parent->mDescriptor->num_methods;
        
        mConstantBaseIndex =
            parent->mConstantBaseIndex + 
            parent->mDescriptor->num_constants;

    }
    LOG_RESOLVE(("+ complete resolve of %s\n", mName));

    SetResolvedState(FULLY_RESOLVED);
    return PR_TRUE;
}        




nsresult
xptiInterfaceEntry::GetName(char **name)
{
    
    *name = (char*) nsMemory::Clone(mName, PL_strlen(mName)+1);
    return *name ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult
xptiInterfaceEntry::GetIID(nsIID **iid)
{
    
    *iid = (nsIID*) nsMemory::Clone(&mIID, sizeof(nsIID));
    return *iid ? NS_OK : NS_ERROR_OUT_OF_MEMORY;
}

nsresult
xptiInterfaceEntry::IsScriptable(PRBool* result)
{
    
    *result = GetScriptableFlag();
    return NS_OK;
}

nsresult
xptiInterfaceEntry::IsFunction(PRBool* result)
{
    if(!EnsureResolved())
        return NS_ERROR_UNEXPECTED;

    *result = XPT_ID_IS_FUNCTION(mDescriptor->flags);
    return NS_OK;
}

nsresult
xptiInterfaceEntry::GetMethodCount(uint16* count)
{
    if(!EnsureResolved())
        return NS_ERROR_UNEXPECTED;
    
    *count = mMethodBaseIndex + 
             mDescriptor->num_methods;
    return NS_OK;
}

nsresult
xptiInterfaceEntry::GetConstantCount(uint16* count)
{
    if(!EnsureResolved())
        return NS_ERROR_UNEXPECTED;

    *count = mConstantBaseIndex + 
             mDescriptor->num_constants;
    return NS_OK;
}

nsresult
xptiInterfaceEntry::GetMethodInfo(uint16 index, const nsXPTMethodInfo** info)
{
    if(!EnsureResolved())
        return NS_ERROR_UNEXPECTED;

    if(index < mMethodBaseIndex)
        return mParent->GetMethodInfo(index, info);

    if(index >= mMethodBaseIndex + 
                mDescriptor->num_methods)
    {
        NS_ERROR("bad param");
        *info = NULL;
        return NS_ERROR_INVALID_ARG;
    }

    
    *info = reinterpret_cast<nsXPTMethodInfo*>
       (&mDescriptor->method_descriptors[index - mMethodBaseIndex]);
    return NS_OK;
}

nsresult
xptiInterfaceEntry::GetMethodInfoForName(const char* methodName, uint16 *index,
                                         const nsXPTMethodInfo** result)
{
    if(!EnsureResolved())
        return NS_ERROR_UNEXPECTED;

    
    for(uint16 i = 0; i < mDescriptor->num_methods; ++i)
    {
        const nsXPTMethodInfo* info;
        info = reinterpret_cast<nsXPTMethodInfo*>
                               (&mDescriptor->
                                        method_descriptors[i]);
        if (PL_strcmp(methodName, info->GetName()) == 0) {
            *index = i + mMethodBaseIndex;
            *result = info;
            return NS_OK;
        }
    }
    
    if(mParent)
        return mParent->GetMethodInfoForName(methodName, index, result);
    else
    {
        *index = 0;
        *result = 0;
        return NS_ERROR_INVALID_ARG;
    }
}

nsresult
xptiInterfaceEntry::GetConstant(uint16 index, const nsXPTConstant** constant)
{
    if(!EnsureResolved())
        return NS_ERROR_UNEXPECTED;

    if(index < mConstantBaseIndex)
        return mParent->GetConstant(index, constant);

    if(index >= mConstantBaseIndex + 
                mDescriptor->num_constants)
    {
        NS_PRECONDITION(0, "bad param");
        *constant = NULL;
        return NS_ERROR_INVALID_ARG;
    }

    
    *constant =
        reinterpret_cast<nsXPTConstant*>
                        (&mDescriptor->
                                const_descriptors[index -
                                    mConstantBaseIndex]);
    return NS_OK;
}



nsresult 
xptiInterfaceEntry::GetEntryForParam(PRUint16 methodIndex, 
                                     const nsXPTParamInfo * param,
                                     xptiInterfaceEntry** entry)
{
    if(!EnsureResolved())
        return NS_ERROR_UNEXPECTED;

    if(methodIndex < mMethodBaseIndex)
        return mParent->GetEntryForParam(methodIndex, param, entry);

    if(methodIndex >= mMethodBaseIndex + 
                      mDescriptor->num_methods)
    {
        NS_ERROR("bad param");
        return NS_ERROR_INVALID_ARG;
    }

    const XPTTypeDescriptor *td = &param->type;

    while (XPT_TDP_TAG(td->prefix) == TD_ARRAY) {
        td = &mDescriptor->additional_types[td->type.additional_type];
    }

    if(XPT_TDP_TAG(td->prefix) != TD_INTERFACE_TYPE) {
        NS_ERROR("not an interface");
        return NS_ERROR_INVALID_ARG;
    }

    xptiInterfaceEntry* theEntry = mTypelib->
        GetEntryAt(td->type.iface - 1);
    
    
    if(!theEntry)
    {
        NS_WARNING("Declared InterfaceInfo not found");
        *entry = nsnull;
        return NS_ERROR_FAILURE;
    }

    *entry = theEntry;
    return NS_OK;
}

nsresult
xptiInterfaceEntry::GetInfoForParam(uint16 methodIndex,
                                    const nsXPTParamInfo *param,
                                    nsIInterfaceInfo** info)
{
    xptiInterfaceEntry* entry;
    nsresult rv = GetEntryForParam(methodIndex, param, &entry);
    if(NS_FAILED(rv))
        return rv;

    xptiInterfaceInfo* theInfo;
    rv = entry->GetInterfaceInfo(&theInfo);    
    if(NS_FAILED(rv))
        return rv;

    *info = static_cast<nsIInterfaceInfo*>(theInfo);
    return NS_OK;
}

nsresult
xptiInterfaceEntry::GetIIDForParam(uint16 methodIndex,
                                   const nsXPTParamInfo* param, nsIID** iid)
{
    xptiInterfaceEntry* entry;
    nsresult rv = GetEntryForParam(methodIndex, param, &entry);
    if(NS_FAILED(rv))
        return rv;
    return entry->GetIID(iid);
}

nsresult
xptiInterfaceEntry::GetIIDForParamNoAlloc(PRUint16 methodIndex, 
                                          const nsXPTParamInfo * param, 
                                          nsIID *iid)
{
    xptiInterfaceEntry* entry;
    nsresult rv = GetEntryForParam(methodIndex, param, &entry);
    if(NS_FAILED(rv))
        return rv;
    *iid = entry->mIID;    
    return NS_OK;
}


nsresult
xptiInterfaceEntry::GetTypeInArray(const nsXPTParamInfo* param,
                                  uint16 dimension,
                                  const XPTTypeDescriptor** type)
{
    NS_ASSERTION(IsFullyResolved(), "bad state");

    const XPTTypeDescriptor *td = &param->type;
    const XPTTypeDescriptor *additional_types =
                mDescriptor->additional_types;

    for (uint16 i = 0; i < dimension; i++) {
        if(XPT_TDP_TAG(td->prefix) != TD_ARRAY) {
            NS_ERROR("bad dimension");
            return NS_ERROR_INVALID_ARG;
        }
        td = &additional_types[td->type.additional_type];
    }

    *type = td;
    return NS_OK;
}

nsresult
xptiInterfaceEntry::GetTypeForParam(uint16 methodIndex,
                                    const nsXPTParamInfo* param,
                                    uint16 dimension,
                                    nsXPTType* type)
{
    if(!EnsureResolved())
        return NS_ERROR_UNEXPECTED;

    if(methodIndex < mMethodBaseIndex)
        return mParent->
            GetTypeForParam(methodIndex, param, dimension, type);

    if(methodIndex >= mMethodBaseIndex + 
                      mDescriptor->num_methods)
    {
        NS_ERROR("bad index");
        return NS_ERROR_INVALID_ARG;
    }

    const XPTTypeDescriptor *td;

    if(dimension) {
        nsresult rv = GetTypeInArray(param, dimension, &td);
        if(NS_FAILED(rv))
            return rv;
    }
    else
        td = &param->type;

    *type = nsXPTType(td->prefix);
    return NS_OK;
}

nsresult
xptiInterfaceEntry::GetSizeIsArgNumberForParam(uint16 methodIndex,
                                               const nsXPTParamInfo* param,
                                               uint16 dimension,
                                               uint8* argnum)
{
    if(!EnsureResolved())
        return NS_ERROR_UNEXPECTED;

    if(methodIndex < mMethodBaseIndex)
        return mParent->
            GetSizeIsArgNumberForParam(methodIndex, param, dimension, argnum);

    if(methodIndex >= mMethodBaseIndex + 
                      mDescriptor->num_methods)
    {
        NS_ERROR("bad index");
        return NS_ERROR_INVALID_ARG;
    }

    const XPTTypeDescriptor *td;

    if(dimension) {
        nsresult rv = GetTypeInArray(param, dimension, &td);
        if(NS_FAILED(rv))
            return rv;
    }
    else
        td = &param->type;

    
    switch (XPT_TDP_TAG(td->prefix)) {
      case TD_ARRAY:
      case TD_PSTRING_SIZE_IS:
      case TD_PWSTRING_SIZE_IS:
        break;
      default:
        NS_ERROR("not a size_is");
        return NS_ERROR_INVALID_ARG;
    }

    *argnum = td->argnum;
    return NS_OK;
}

nsresult
xptiInterfaceEntry::GetLengthIsArgNumberForParam(uint16 methodIndex,
                                                 const nsXPTParamInfo* param,
                                                 uint16 dimension,
                                                 uint8* argnum)
{
    if(!EnsureResolved())
        return NS_ERROR_UNEXPECTED;

    if(methodIndex < mMethodBaseIndex)
        return mParent->
            GetLengthIsArgNumberForParam(methodIndex, param, dimension, argnum);

    if(methodIndex >= mMethodBaseIndex + 
                      mDescriptor->num_methods)
    {
        NS_ERROR("bad index");
        return NS_ERROR_INVALID_ARG;
    }

    const XPTTypeDescriptor *td;

    if(dimension) {
        nsresult rv = GetTypeInArray(param, dimension, &td);
        if(NS_FAILED(rv)) {
            return rv;
        }
    }
    else
        td = &param->type;

    
    switch (XPT_TDP_TAG(td->prefix)) {
      case TD_ARRAY:
      case TD_PSTRING_SIZE_IS:
      case TD_PWSTRING_SIZE_IS:
        break;
      default:
        NS_ERROR("not a length_is");
        return NS_ERROR_INVALID_ARG;
    }

    *argnum = td->argnum2;
    return NS_OK;
}

nsresult
xptiInterfaceEntry::GetInterfaceIsArgNumberForParam(uint16 methodIndex,
                                                    const nsXPTParamInfo* param,
                                                    uint8* argnum)
{
    if(!EnsureResolved())
        return NS_ERROR_UNEXPECTED;

    if(methodIndex < mMethodBaseIndex)
        return mParent->
            GetInterfaceIsArgNumberForParam(methodIndex, param, argnum);

    if(methodIndex >= mMethodBaseIndex + 
                      mDescriptor->num_methods)
    {
        NS_ERROR("bad index");
        return NS_ERROR_INVALID_ARG;
    }

    const XPTTypeDescriptor *td = &param->type;

    while (XPT_TDP_TAG(td->prefix) == TD_ARRAY) {
        td = &mDescriptor->
                                additional_types[td->type.additional_type];
    }

    if(XPT_TDP_TAG(td->prefix) != TD_INTERFACE_IS_TYPE) {
        NS_ERROR("not an iid_is");
        return NS_ERROR_INVALID_ARG;
    }

    *argnum = td->argnum;
    return NS_OK;
}


nsresult 
xptiInterfaceEntry::IsIID(const nsIID * IID, PRBool *_retval)
{
    
    *_retval = mIID.Equals(*IID);
    return NS_OK;
}


nsresult 
xptiInterfaceEntry::GetNameShared(const char **name)
{
    
    *name = mName;
    return NS_OK;
}


nsresult 
xptiInterfaceEntry::GetIIDShared(const nsIID * *iid)
{
    
    *iid = &mIID;
    return NS_OK;
}


nsresult 
xptiInterfaceEntry::HasAncestor(const nsIID * iid, PRBool *_retval)
{
    *_retval = PR_FALSE;

    for(xptiInterfaceEntry* current = this; 
        current;
        current = current->mParent)
    {
        if(current->mIID.Equals(*iid))
        {
            *_retval = PR_TRUE;
            break;
        }
        if(!current->EnsureResolved())
            return NS_ERROR_UNEXPECTED;
    }

    return NS_OK;
}



nsresult 
xptiInterfaceEntry::GetInterfaceInfo(xptiInterfaceInfo** info)
{
#ifdef DEBUG
    xptiInterfaceInfoManager::GetSingleton()->GetWorkingSet()->mTableLock.
        AssertCurrentThreadOwns();
#endif
    LOG_INFO_MONITOR_ENTRY;

    if(!mInfo)
    {
        mInfo = new xptiInterfaceInfo(this);
        if(!mInfo)
        {
            *info = nsnull;    
            return NS_ERROR_OUT_OF_MEMORY;
        }
    }
    
    NS_ADDREF(*info = mInfo);
    return NS_OK;    
}
    
void     
xptiInterfaceEntry::LockedInvalidateInterfaceInfo()
{
    if(mInfo)
    {
        mInfo->Invalidate(); 
        mInfo = nsnull;
    }
}

PRBool
xptiInterfaceInfo::BuildParent()
{
    mozilla::MutexAutoLock lock(xptiInterfaceInfoManager::GetSingleton()->
                                    GetWorkingSet()->mTableLock);
    NS_ASSERTION(mEntry && 
                 mEntry->IsFullyResolved() && 
                 !mParent &&
                 mEntry->Parent(),
                "bad BuildParent call");
    return NS_SUCCEEDED(mEntry->Parent()->GetInterfaceInfo(&mParent));
}



NS_IMPL_QUERY_INTERFACE1(xptiInterfaceInfo, nsIInterfaceInfo)

xptiInterfaceInfo::xptiInterfaceInfo(xptiInterfaceEntry* entry)
    : mEntry(entry), mParent(nsnull)
{
    LOG_INFO_CREATE(this);
}

xptiInterfaceInfo::~xptiInterfaceInfo() 
{
    LOG_INFO_DESTROY(this);
    NS_IF_RELEASE(mParent); 
    NS_ASSERTION(!mEntry, "bad state in dtor");
}

nsrefcnt
xptiInterfaceInfo::AddRef(void)
{
    nsrefcnt cnt = NS_AtomicIncrementRefcnt(mRefCnt);
    NS_LOG_ADDREF(this, cnt, "xptiInterfaceInfo", sizeof(*this));
    return cnt;
}

nsrefcnt
xptiInterfaceInfo::Release(void)
{
    xptiInterfaceEntry* entry = mEntry;
    nsrefcnt cnt = NS_AtomicDecrementRefcnt(mRefCnt);
    NS_LOG_RELEASE(this, cnt, "xptiInterfaceInfo");
    if(!cnt)
    {
        
        
        {
            mozilla::MutexAutoLock lock(xptiInterfaceInfoManager::
                                            GetSingleton()->GetWorkingSet()->
                                            mTableLock);
            LOG_INFO_MONITOR_ENTRY;

            
            
            
            
            
            
            
            if(entry && !entry->InterfaceInfoEquals(this))
                return 0;

            
            
            
            if(mRefCnt)
                return 1;

            if(mEntry)
            {
                mEntry->LockedInterfaceInfoDeathNotification();
                mEntry = nsnull;
            }
        }

        delete this;
        return 0;    
    }
    return cnt;
}


