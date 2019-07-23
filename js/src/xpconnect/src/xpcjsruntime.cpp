









































#include "xpcprivate.h"



const char* XPCJSRuntime::mStrings[] = {
    "constructor",          
    "toString",             
    "toSource",             
    "lastResult",           
    "returnCode",           
    "value",                
    "QueryInterface",       
    "Components",           
    "wrappedJSObject",      
    "Object",               
    "Function",             
    "prototype",            
    "createInstance",       
    "item"                  
#ifdef XPC_IDISPATCH_SUPPORT
    , "GeckoActiveXObject"  
    , "COMObject"           
    , "supports"            
#endif
};




static JSContextCallback gOldJSContextCallback;


static JSGCCallback gOldJSGCCallback;


struct JSDyingJSObjectData
{
    JSContext* cx;
    nsVoidArray* array;
};

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
WrappedJSDyingJSObjectFinder(JSDHashTable *table, JSDHashEntryHdr *hdr,
                uint32 number, void *arg)
{
    JSDyingJSObjectData* data = (JSDyingJSObjectData*) arg;
    nsXPCWrappedJS* wrapper = ((JSObject2WrappedJSMap::Entry*)hdr)->value;
    NS_ASSERTION(wrapper, "found a null JS wrapper!");

    
    while(wrapper)
    {
        if(wrapper->IsSubjectToFinalization())
        {
            if(JS_IsAboutToBeFinalized(data->cx, wrapper->GetJSObject()))
                data->array->AppendElement(wrapper);
        }
        wrapper = wrapper->GetNextWrapper();
    }
    return JS_DHASH_NEXT;
}

struct CX_AND_XPCRT_Data
{
    JSContext* cx;
    XPCJSRuntime* rt;
};

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
NativeInterfaceGC(JSDHashTable *table, JSDHashEntryHdr *hdr,
                  uint32 number, void *arg)
{
    CX_AND_XPCRT_Data* data = (CX_AND_XPCRT_Data*) arg;
    ((IID2NativeInterfaceMap::Entry*)hdr)->value->
            DealWithDyingGCThings(data->cx, data->rt);
    return JS_DHASH_NEXT;
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
NativeInterfaceSweeper(JSDHashTable *table, JSDHashEntryHdr *hdr,
                       uint32 number, void *arg)
{
    CX_AND_XPCRT_Data* data = (CX_AND_XPCRT_Data*) arg;
    XPCNativeInterface* iface = ((IID2NativeInterfaceMap::Entry*)hdr)->value;
    if(iface->IsMarked())
    {
        iface->Unmark();
        return JS_DHASH_NEXT;
    }

#ifdef XPC_REPORT_NATIVE_INTERFACE_AND_SET_FLUSHING
    printf("- Destroying XPCNativeInterface for %s\n",
            JS_GetStringBytes(JSVAL_TO_STRING(iface->GetName())));
#endif

    XPCNativeInterface::DestroyInstance(data->cx, data->rt, iface);
    return JS_DHASH_REMOVE;
}






JS_STATIC_DLL_CALLBACK(JSDHashOperator)
NativeUnMarkedSetRemover(JSDHashTable *table, JSDHashEntryHdr *hdr,
                         uint32 number, void *arg)
{
    XPCNativeSet* set = ((ClassInfo2NativeSetMap::Entry*)hdr)->value;
    if(set->IsMarked())
        return JS_DHASH_NEXT;
    return JS_DHASH_REMOVE;
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
NativeSetSweeper(JSDHashTable *table, JSDHashEntryHdr *hdr,
                 uint32 number, void *arg)
{
    XPCNativeSet* set = ((NativeSetMap::Entry*)hdr)->key_value;
    if(set->IsMarked())
    {
        set->Unmark();
        return JS_DHASH_NEXT;
    }

#ifdef XPC_REPORT_NATIVE_INTERFACE_AND_SET_FLUSHING
    printf("- Destroying XPCNativeSet for:\n");
    PRUint16 count = set->GetInterfaceCount();
    for(PRUint16 k = 0; k < count; k++)
    {
        XPCNativeInterface* iface = set->GetInterfaceAt(k);
        printf("    %s\n",JS_GetStringBytes(JSVAL_TO_STRING(iface->GetName())));
    }
#endif

    XPCNativeSet::DestroyInstance(set);
    return JS_DHASH_REMOVE;
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
JSClassSweeper(JSDHashTable *table, JSDHashEntryHdr *hdr,
               uint32 number, void *arg)
{
    XPCNativeScriptableShared* shared =
        ((XPCNativeScriptableSharedMap::Entry*) hdr)->key;
    if(shared->IsMarked())
    {
#ifdef off_XPC_REPORT_JSCLASS_FLUSHING
        printf("+ Marked XPCNativeScriptableShared for: %s @ %x\n",
               shared->GetJSClass()->name,
               shared->GetJSClass());
#endif
        shared->Unmark();
        return JS_DHASH_NEXT;
    }

#ifdef XPC_REPORT_JSCLASS_FLUSHING
    printf("- Destroying XPCNativeScriptableShared for: %s @ %x\n",
           shared->GetJSClass()->name,
           shared->GetJSClass());
#endif

    delete shared;
    return JS_DHASH_REMOVE;
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
DyingProtoKiller(JSDHashTable *table, JSDHashEntryHdr *hdr,
                 uint32 number, void *arg)
{
    XPCWrappedNativeProto* proto =
        (XPCWrappedNativeProto*)((JSDHashEntryStub*)hdr)->key;
    delete proto;
    return JS_DHASH_REMOVE;
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
DetachedWrappedNativeProtoMarker(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                 uint32 number, void *arg)
{
    XPCWrappedNativeProto* proto = 
        (XPCWrappedNativeProto*)((JSDHashEntryStub*)hdr)->key;

    proto->Mark();
    return JS_DHASH_NEXT;
}


JS_STATIC_DLL_CALLBACK(JSBool)
ContextCallback(JSContext *cx, uintN operation)
{
    XPCJSRuntime* self = nsXPConnect::GetRuntime();
    if (self)
    {
        if (operation == JSCONTEXT_NEW)
        {
            XPCPerThreadData* tls = XPCPerThreadData::GetData();
            if(tls)
            {
                JS_SetThreadStackLimit(cx, tls->GetStackLimit());
            }
        }
    }

    return gOldJSContextCallback
           ? gOldJSContextCallback(cx, operation)
           : JS_TRUE;
}


void XPCJSRuntime::TraceJS(JSTracer* trc, void* data)
{
    XPCJSRuntime* self = (XPCJSRuntime*)data;

    
    
    if(!self->GetXPConnect()->IsShuttingDown())
    {
        PRLock* threadLock = XPCPerThreadData::GetLock();
        if(threadLock)
        { 
            nsAutoLock lock(threadLock);

            XPCPerThreadData* iterp = nsnull;
            XPCPerThreadData* thread;

            while(nsnull != (thread =
                             XPCPerThreadData::IterateThreads(&iterp)))
            {
                
                thread->TraceJS(trc);
            }
        }
    }

    XPCWrappedNativeScope::TraceJS(trc, self);

    for (XPCRootSetElem *e = self->mVariantRoots; e ; e = e->GetNextRoot())
        NS_STATIC_CAST(XPCTraceableVariant*, e)->TraceJS(trc);

    for (XPCRootSetElem *e = self->mWrappedJSRoots; e ; e = e->GetNextRoot())
        NS_STATIC_CAST(nsXPCWrappedJS*, e)->TraceJS(trc);

    for (XPCRootSetElem *e = self->mObjectHolderRoots; e ; e = e->GetNextRoot())
        NS_STATIC_CAST(XPCJSObjectHolder*, e)->TraceJS(trc);
}


JSBool XPCJSRuntime::GCCallback(JSContext *cx, JSGCStatus status)
{
    nsVoidArray* dyingWrappedJSArray;

    XPCJSRuntime* self = nsXPConnect::GetRuntime();
    if(self)
    {
        switch(status)
        {
            case JSGC_BEGIN:
            {
                if(self->GetMainThreadOnlyGC() && !NS_IsMainThread())
                {
                    return JS_FALSE;
                }
                break;
            }
            case JSGC_MARK_END:
            {
                NS_ASSERTION(!self->mDoingFinalization, "bad state");
    
                
                { 
                    XPCAutoLock lock(self->GetMapLock());
                    NS_ASSERTION(!self->mThreadRunningGC, "bad state");
                    self->mThreadRunningGC = PR_GetCurrentThread();
                }

                dyingWrappedJSArray = &self->mWrappedJSToReleaseArray;
                {
                    XPCLock* lock = self->GetMainThreadOnlyGC() ?
                                    nsnull : self->GetMapLock();

                    XPCAutoLock al(lock); 
                    JSDyingJSObjectData data = {cx, dyingWrappedJSArray};

                    
                    
                    
                    
                    
                    
                    self->mWrappedJSMap->
                        Enumerate(WrappedJSDyingJSObjectFinder, &data);
                }

                
                
                
                
                CX_AND_XPCRT_Data data = {cx, self};

                self->mIID2NativeInterfaceMap->
                    Enumerate(NativeInterfaceGC, &data);

                
                XPCWrappedNativeScope::FinishedMarkPhaseOfGC(cx, self);

                self->mDoingFinalization = JS_TRUE;
                break;
            }
            case JSGC_FINALIZE_END:
            {
                NS_ASSERTION(self->mDoingFinalization, "bad state");
                self->mDoingFinalization = JS_FALSE;

                
                

                dyingWrappedJSArray = &self->mWrappedJSToReleaseArray;
                XPCLock* mapLock = self->GetMainThreadOnlyGC() ?
                                   nsnull : self->GetMapLock();
                while(1)
                {
                    nsXPCWrappedJS* wrapper;
                    {
                        XPCAutoLock al(mapLock); 
                        PRInt32 count = dyingWrappedJSArray->Count();
                        if(!count)
                        {
                            dyingWrappedJSArray->Compact();
                            break;
                        }
                        wrapper = NS_STATIC_CAST(nsXPCWrappedJS*,
                                    dyingWrappedJSArray->ElementAt(count-1));
                        dyingWrappedJSArray->RemoveElementAt(count-1);
                    }
                    NS_RELEASE(wrapper);
                }


#ifdef XPC_REPORT_NATIVE_INTERFACE_AND_SET_FLUSHING
                printf("--------------------------------------------------------------\n");
                int setsBefore = (int) self->mNativeSetMap->Count();
                int ifacesBefore = (int) self->mIID2NativeInterfaceMap->Count();
#endif

                
                

                
                XPCWrappedNativeScope::MarkAllWrappedNativesAndProtos();

                self->mDetachedWrappedNativeProtoMap->
                    Enumerate(DetachedWrappedNativeProtoMarker, nsnull);

                
                
                
                
                
                

                
                
                if(!self->GetXPConnect()->IsShuttingDown())
                {
                    PRLock* threadLock = XPCPerThreadData::GetLock();
                    if(threadLock)
                    { 
                        nsAutoLock lock(threadLock);

                        XPCPerThreadData* iterp = nsnull;
                        XPCPerThreadData* thread;

                        while(nsnull != (thread =
                                     XPCPerThreadData::IterateThreads(&iterp)))
                        {
                            
                            thread->MarkAutoRootsAfterJSFinalize();

                            XPCCallContext* ccxp = thread->GetCallContext();
                            while(ccxp)
                            {
                                
                                
                                
                                
                                if(ccxp->CanGetSet())
                                {
                                    XPCNativeSet* set = ccxp->GetSet();
                                    if(set)
                                        set->Mark();
                                }
                                if(ccxp->CanGetInterface())
                                {
                                    XPCNativeInterface* iface = ccxp->GetInterface();
                                    if(iface)
                                        iface->Mark();
                                }
                                ccxp = ccxp->GetPrevCallContext();
                            }
                        }
                    }
                }

                

                
                
                
                if(!self->GetXPConnect()->IsShuttingDown())
                {
                    self->mNativeScriptableSharedMap->
                        Enumerate(JSClassSweeper, nsnull);
                }

                self->mClassInfo2NativeSetMap->
                    Enumerate(NativeUnMarkedSetRemover, nsnull);

                self->mNativeSetMap->
                    Enumerate(NativeSetSweeper, nsnull);

                CX_AND_XPCRT_Data data = {cx, self};

                self->mIID2NativeInterfaceMap->
                    Enumerate(NativeInterfaceSweeper, &data);

#ifdef DEBUG
                XPCWrappedNativeScope::ASSERT_NoInterfaceSetsAreMarked();
#endif

#ifdef XPC_REPORT_NATIVE_INTERFACE_AND_SET_FLUSHING
                int setsAfter = (int) self->mNativeSetMap->Count();
                int ifacesAfter = (int) self->mIID2NativeInterfaceMap->Count();

                printf("\n");
                printf("XPCNativeSets:        before: %d  collected: %d  remaining: %d\n",
                       setsBefore, setsBefore - setsAfter, setsAfter);
                printf("XPCNativeInterfaces:  before: %d  collected: %d  remaining: %d\n",
                       ifacesBefore, ifacesBefore - ifacesAfter, ifacesAfter);
                printf("--------------------------------------------------------------\n");
#endif

                
                XPCWrappedNativeScope::FinishedFinalizationPhaseOfGC(cx);

                
                
                
                
                
                
                
                
                
                
                

                
                
                if(!self->GetXPConnect()->IsShuttingDown())
                {
                    PRLock* threadLock = XPCPerThreadData::GetLock();
                    if(threadLock)
                    {
                        
                        
                        { 
                            nsAutoLock lock(threadLock);

                            XPCPerThreadData* iterp = nsnull;
                            XPCPerThreadData* thread;

                            while(nsnull != (thread =
                                     XPCPerThreadData::IterateThreads(&iterp)))
                            {
                                XPCCallContext* ccxp = thread->GetCallContext();
                                while(ccxp)
                                {
                                    
                                    
                                    
                                    
                                    if(ccxp->CanGetTearOff())
                                    {
                                        XPCWrappedNativeTearOff* to = 
                                            ccxp->GetTearOff();
                                        if(to)
                                            to->Mark();
                                    }
                                    ccxp = ccxp->GetPrevCallContext();
                                }
                            }
                        }
    
                        
                        XPCWrappedNativeScope::SweepAllWrappedNativeTearOffs();
                    }
                }

                
                
                
                
                
                
                
                
                
                
                
                
                

                self->mDyingWrappedNativeProtoMap->
                    Enumerate(DyingProtoKiller, nsnull);


                
                
                { 
                    XPCAutoLock lock(self->GetMapLock());
                    NS_ASSERTION(self->mThreadRunningGC == PR_GetCurrentThread(), "bad state");
                    self->mThreadRunningGC = nsnull;
                    xpc_NotifyAll(self->GetMapLock());
                }

                break;
            }
            case JSGC_END:
            {
                
                
                

                XPCLock* lock = self->GetMainThreadOnlyGC() ?
                                nsnull : self->GetMapLock();

                
                if(self->GetDeferReleases())
                {
                    nsVoidArray* array = &self->mNativesToReleaseArray;
#ifdef XPC_TRACK_DEFERRED_RELEASES
                    printf("XPC - Begin deferred Release of %d nsISupports pointers\n",
                           array->Count());
#endif
                    while(1)
                    {
                        nsISupports* obj;
                        {
                            XPCAutoLock al(lock); 
                            PRInt32 count = array->Count();
                            if(!count)
                            {
                                array->Compact();
                                break;
                            }
                            obj = NS_REINTERPRET_CAST(nsISupports*,
                                    array->ElementAt(count-1));
                            array->RemoveElementAt(count-1);
                        }
                        NS_RELEASE(obj);
                    }
#ifdef XPC_TRACK_DEFERRED_RELEASES
                    printf("XPC - End deferred Releases\n");
#endif
                }
                break;
            }
            default:
                break;
        }
    }

    
    return gOldJSGCCallback ? gOldJSGCCallback(cx, status) : JS_TRUE;
}



#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
JS_STATIC_DLL_CALLBACK(JSDHashOperator)
DEBUG_WrapperChecker(JSDHashTable *table, JSDHashEntryHdr *hdr,
                     uint32 number, void *arg)
{
    XPCWrappedNative* wrapper = (XPCWrappedNative*)((JSDHashEntryStub*)hdr)->key;
    NS_ASSERTION(!wrapper->IsValid(), "found a 'valid' wrapper!");
    ++ *((int*)arg);
    return JS_DHASH_NEXT;
}
#endif

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
WrappedJSShutdownMarker(JSDHashTable *table, JSDHashEntryHdr *hdr,
                        uint32 number, void *arg)
{
    JSRuntime* rt = (JSRuntime*) arg;
    nsXPCWrappedJS* wrapper = ((JSObject2WrappedJSMap::Entry*)hdr)->value;
    NS_ASSERTION(wrapper, "found a null JS wrapper!");
    NS_ASSERTION(wrapper->IsValid(), "found an invalid JS wrapper!");
    wrapper->SystemIsBeingShutDown(rt);
    return JS_DHASH_NEXT;
}

JS_STATIC_DLL_CALLBACK(JSDHashOperator)
DetachedWrappedNativeProtoShutdownMarker(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                         uint32 number, void *arg)
{
    XPCWrappedNativeProto* proto = 
        (XPCWrappedNativeProto*)((JSDHashEntryStub*)hdr)->key;

    proto->SystemIsBeingShutDown(*((XPCCallContext*)arg));
    return JS_DHASH_NEXT;
}

void XPCJSRuntime::SystemIsBeingShutDown(XPCCallContext* ccx)
{
    if(mDetachedWrappedNativeProtoMap)
        mDetachedWrappedNativeProtoMap->
            Enumerate(DetachedWrappedNativeProtoShutdownMarker, ccx);
}

XPCJSRuntime::~XPCJSRuntime()
{
#ifdef XPC_DUMP_AT_SHUTDOWN
    {
    
    JSContext* iter = nsnull;
    int count = 0;
    while(JS_ContextIterator(mJSRuntime, &iter))
        count ++;
    if(count)
        printf("deleting XPCJSRuntime with %d live JSContexts\n", count);
    }
#endif

    

    if(mContextMap)
    {
        PurgeXPCContextList();
        delete mContextMap;
    }

    if(mWrappedJSMap)
    {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32 count = mWrappedJSMap->Count();
        if(count)
            printf("deleting XPCJSRuntime with %d live wrapped JSObject\n", (int)count);
#endif
        mWrappedJSMap->Enumerate(WrappedJSShutdownMarker, mJSRuntime);
        delete mWrappedJSMap;
    }

    if(mWrappedJSClassMap)
    {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32 count = mWrappedJSClassMap->Count();
        if(count)
            printf("deleting XPCJSRuntime with %d live nsXPCWrappedJSClass\n", (int)count);
#endif
        delete mWrappedJSClassMap;
    }

    if(mIID2NativeInterfaceMap)
    {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32 count = mIID2NativeInterfaceMap->Count();
        if(count)
            printf("deleting XPCJSRuntime with %d live XPCNativeInterfaces\n", (int)count);
#endif
        delete mIID2NativeInterfaceMap;
    }

    if(mClassInfo2NativeSetMap)
    {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32 count = mClassInfo2NativeSetMap->Count();
        if(count)
            printf("deleting XPCJSRuntime with %d live XPCNativeSets\n", (int)count);
#endif
        delete mClassInfo2NativeSetMap;
    }

    if(mNativeSetMap)
    {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32 count = mNativeSetMap->Count();
        if(count)
            printf("deleting XPCJSRuntime with %d live XPCNativeSets\n", (int)count);
#endif
        delete mNativeSetMap;
    }

    if(mMapLock)
        XPCAutoLock::DestroyLock(mMapLock);
    NS_IF_RELEASE(mJSRuntimeService);

    if(mThisTranslatorMap)
    {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32 count = mThisTranslatorMap->Count();
        if(count)
            printf("deleting XPCJSRuntime with %d live ThisTranslator\n", (int)count);
#endif
        delete mThisTranslatorMap;
    }

#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
    if(DEBUG_WrappedNativeHashtable)
    {
        int LiveWrapperCount = 0;
        JS_DHashTableEnumerate(DEBUG_WrappedNativeHashtable,
                               DEBUG_WrapperChecker, &LiveWrapperCount);
        if(LiveWrapperCount)
            printf("deleting XPCJSRuntime with %d live XPCWrappedNative (found in wrapper check)\n", (int)LiveWrapperCount);
        JS_DHashTableDestroy(DEBUG_WrappedNativeHashtable);
    }
#endif

    if(mNativeScriptableSharedMap)
    {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32 count = mNativeScriptableSharedMap->Count();
        if(count)
            printf("deleting XPCJSRuntime with %d live XPCNativeScriptableShared\n", (int)count);
#endif
        delete mNativeScriptableSharedMap;
    }

    if(mDyingWrappedNativeProtoMap)
    {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32 count = mDyingWrappedNativeProtoMap->Count();
        if(count)
            printf("deleting XPCJSRuntime with %d live but dying XPCWrappedNativeProto\n", (int)count);
#endif
        delete mDyingWrappedNativeProtoMap;
    }

    if(mDetachedWrappedNativeProtoMap)
    {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32 count = mDetachedWrappedNativeProtoMap->Count();
        if(count)
            printf("deleting XPCJSRuntime with %d live detached XPCWrappedNativeProto\n", (int)count);
#endif
        delete mDetachedWrappedNativeProtoMap;
    }

    if(mExplicitNativeWrapperMap)
    {
#ifdef XPC_DUMP_AT_SHUTDOWN
        uint32 count = mExplicitNativeWrapperMap->Count();
        if(count)
            printf("deleting XPCJSRuntime with %d live explicit XPCNativeWrapper\n", (int)count);
#endif
        delete mExplicitNativeWrapperMap;
    }

    
    XPCStringConvert::ShutdownDOMStringFinalizer();

    XPCConvert::RemoveXPCOMUCStringFinalizer();

    gOldJSGCCallback = NULL;
    gOldJSContextCallback = NULL;
}

XPCJSRuntime::XPCJSRuntime(nsXPConnect* aXPConnect,
                           nsIJSRuntimeService* aJSRuntimeService)
 : mXPConnect(aXPConnect),
   mJSRuntime(nsnull),
   mJSRuntimeService(aJSRuntimeService),
   mContextMap(JSContext2XPCContextMap::newMap(XPC_CONTEXT_MAP_SIZE)),
   mWrappedJSMap(JSObject2WrappedJSMap::newMap(XPC_JS_MAP_SIZE)),
   mWrappedJSClassMap(IID2WrappedJSClassMap::newMap(XPC_JS_CLASS_MAP_SIZE)),
   mIID2NativeInterfaceMap(IID2NativeInterfaceMap::newMap(XPC_NATIVE_INTERFACE_MAP_SIZE)),
   mClassInfo2NativeSetMap(ClassInfo2NativeSetMap::newMap(XPC_NATIVE_SET_MAP_SIZE)),
   mNativeSetMap(NativeSetMap::newMap(XPC_NATIVE_SET_MAP_SIZE)),
   mThisTranslatorMap(IID2ThisTranslatorMap::newMap(XPC_THIS_TRANSLATOR_MAP_SIZE)),
   mNativeScriptableSharedMap(XPCNativeScriptableSharedMap::newMap(XPC_NATIVE_JSCLASS_MAP_SIZE)),
   mDyingWrappedNativeProtoMap(XPCWrappedNativeProtoMap::newMap(XPC_DYING_NATIVE_PROTO_MAP_SIZE)),
   mDetachedWrappedNativeProtoMap(XPCWrappedNativeProtoMap::newMap(XPC_DETACHED_NATIVE_PROTO_MAP_SIZE)),
   mExplicitNativeWrapperMap(XPCNativeWrapperMap::newMap(XPC_NATIVE_WRAPPER_MAP_SIZE)),
   mMapLock(XPCAutoLock::NewLock("XPCJSRuntime::mMapLock")),
   mThreadRunningGC(nsnull),
   mWrappedJSToReleaseArray(),
   mNativesToReleaseArray(),
   mMainThreadOnlyGC(JS_FALSE),
   mDeferReleases(JS_FALSE),
   mDoingFinalization(JS_FALSE),
   mVariantRoots(nsnull),
   mWrappedJSRoots(nsnull),
   mObjectHolderRoots(nsnull)
{
#ifdef XPC_CHECK_WRAPPERS_AT_SHUTDOWN
    DEBUG_WrappedNativeHashtable =
        JS_NewDHashTable(JS_DHashGetStubOps(), nsnull,
                         sizeof(JSDHashEntryStub), 128);
#endif

    
    mStrIDs[0] = 0;

    if(mJSRuntimeService)
    {
        NS_ADDREF(mJSRuntimeService);
        mJSRuntimeService->GetRuntime(&mJSRuntime);
    }

    NS_ASSERTION(!gOldJSGCCallback, "XPCJSRuntime created more than once");
    if(mJSRuntime)
    {
        gOldJSContextCallback = JS_SetContextCallback(mJSRuntime,
                                                      ContextCallback);
        gOldJSGCCallback = JS_SetGCCallbackRT(mJSRuntime, GCCallback);
        JS_SetExtraGCRoots(mJSRuntime, TraceJS, this);
    }

    
#ifdef DEBUG
    if(mJSRuntime && !JS_GetGlobalDebugHooks(mJSRuntime)->debuggerHandler)
        xpc_InstallJSDebuggerKeywordHandler(mJSRuntime);
#endif
}


XPCJSRuntime*
XPCJSRuntime::newXPCJSRuntime(nsXPConnect* aXPConnect,
                              nsIJSRuntimeService* aJSRuntimeService)
{
    NS_PRECONDITION(aXPConnect,"bad param");
    NS_PRECONDITION(aJSRuntimeService,"bad param");

    XPCJSRuntime* self;

    self = new XPCJSRuntime(aXPConnect,
                            aJSRuntimeService);

    if(self                                  &&
       self->GetJSRuntime()                  &&
       self->GetContextMap()                 &&
       self->GetWrappedJSMap()               &&
       self->GetWrappedJSClassMap()          &&
       self->GetIID2NativeInterfaceMap()     &&
       self->GetClassInfo2NativeSetMap()     &&
       self->GetNativeSetMap()               &&
       self->GetThisTranslatorMap()          &&
       self->GetNativeScriptableSharedMap()  &&
       self->GetDyingWrappedNativeProtoMap() &&
       self->GetExplicitNativeWrapperMap()   &&
       self->GetMapLock())
    {
        return self;
    }
    delete self;
    return nsnull;
}

XPCContext*
XPCJSRuntime::GetXPCContext(JSContext* cx)
{
    XPCContext* xpcc;

    

    { 
        XPCAutoLock lock(GetMapLock());
        xpcc = mContextMap->Find(cx);
    }

    
    if(!xpcc)
        xpcc = SyncXPCContextList(cx);
    return xpcc;
}


JS_STATIC_DLL_CALLBACK(JSDHashOperator)
SweepContextsCB(JSDHashTable *table, JSDHashEntryHdr *hdr,
                uint32 number, void *arg)
{
    XPCContext* xpcc = ((JSContext2XPCContextMap::Entry*)hdr)->value;
    if(xpcc->IsMarked())
    {
        xpcc->Unmark();
        return JS_DHASH_NEXT;
    }

    
    delete xpcc;
    return JS_DHASH_REMOVE;
}

XPCContext*
XPCJSRuntime::SyncXPCContextList(JSContext* cx )
{
    
    XPCAutoLock lock(GetMapLock());

    XPCContext* found = nsnull;

    
    JSContext *cur, *iter = nsnull;
    while(nsnull != (cur = JS_ContextIterator(mJSRuntime, &iter)))
    {
        XPCContext* xpcc = mContextMap->Find(cur);

        if(!xpcc)
        {
            xpcc = XPCContext::newXPCContext(this, cur);
            if(xpcc)
                mContextMap->Add(xpcc);
        }
        if(xpcc)
        {
            xpcc->Mark();
        }

        
        if(!mStrIDs[0])
        {
            JSAutoRequest ar(cur);
            GenerateStringIDs(cur);
        }

        if(cx && cx == cur)
            found = xpcc;
    }
    
    mContextMap->Enumerate(SweepContextsCB, 0);

    XPCPerThreadData* tls = XPCPerThreadData::GetData();
    if(tls)
    {
        if(found)
            tls->SetRecentContext(cx, found);
        else
            tls->ClearRecentContext();
    }

    return found;
}


JS_STATIC_DLL_CALLBACK(JSDHashOperator)
PurgeContextsCB(JSDHashTable *table, JSDHashEntryHdr *hdr,
                uint32 number, void *arg)
{
    delete ((JSContext2XPCContextMap::Entry*)hdr)->value;
    return JS_DHASH_REMOVE;
}

void
XPCJSRuntime::PurgeXPCContextList()
{
    
    XPCAutoLock lock(GetMapLock());

    
    mContextMap->Enumerate(PurgeContextsCB, nsnull);
}

JSBool
XPCJSRuntime::GenerateStringIDs(JSContext* cx)
{
    NS_PRECONDITION(!mStrIDs[0],"string ids generated twice!");
    for(uintN i = 0; i < IDX_TOTAL_COUNT; i++)
    {
        JSString* str = JS_InternString(cx, mStrings[i]);
        if(!str || !JS_ValueToId(cx, STRING_TO_JSVAL(str), &mStrIDs[i]))
        {
            mStrIDs[0] = 0;
            return JS_FALSE;
        }

        mStrJSVals[i] = STRING_TO_JSVAL(str);
    }
    return JS_TRUE;
}

JSBool
XPCJSRuntime::DeferredRelease(nsISupports* obj)
{
    NS_ASSERTION(obj, "bad param");
    NS_ASSERTION(GetDeferReleases(), "bad call");

    XPCLock* lock = GetMainThreadOnlyGC() ? nsnull : GetMapLock();
    {
        XPCAutoLock al(lock); 
        if(!mNativesToReleaseArray.Count())
        {
            
            
            
            mNativesToReleaseArray.SizeTo(256);
        }
        return mNativesToReleaseArray.AppendElement(obj);
    }        
}



#ifdef DEBUG
JS_STATIC_DLL_CALLBACK(JSDHashOperator)
ContextMapDumpEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                         uint32 number, void *arg)
{
    ((JSContext2XPCContextMap::Entry*)hdr)->value->DebugDump(*(PRInt16*)arg);
    return JS_DHASH_NEXT;
}
JS_STATIC_DLL_CALLBACK(JSDHashOperator)
WrappedJSClassMapDumpEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                                uint32 number, void *arg)
{
    ((IID2WrappedJSClassMap::Entry*)hdr)->value->DebugDump(*(PRInt16*)arg);
    return JS_DHASH_NEXT;
}
JS_STATIC_DLL_CALLBACK(JSDHashOperator)
WrappedJSMapDumpEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                           uint32 number, void *arg)
{
    ((JSObject2WrappedJSMap::Entry*)hdr)->value->DebugDump(*(PRInt16*)arg);
    return JS_DHASH_NEXT;
}
JS_STATIC_DLL_CALLBACK(JSDHashOperator)
NativeSetDumpEnumerator(JSDHashTable *table, JSDHashEntryHdr *hdr,
                        uint32 number, void *arg)
{
    ((NativeSetMap::Entry*)hdr)->key_value->DebugDump(*(PRInt16*)arg);
    return JS_DHASH_NEXT;
}
#endif

void
XPCJSRuntime::DebugDump(PRInt16 depth)
{
#ifdef DEBUG
    depth--;
    XPC_LOG_ALWAYS(("XPCJSRuntime @ %x", this));
        XPC_LOG_INDENT();
        XPC_LOG_ALWAYS(("mXPConnect @ %x", mXPConnect));
        XPC_LOG_ALWAYS(("mJSRuntime @ %x", mJSRuntime));
        XPC_LOG_ALWAYS(("mMapLock @ %x", mMapLock));
        XPC_LOG_ALWAYS(("mJSRuntimeService @ %x", mJSRuntimeService));

        XPC_LOG_ALWAYS(("mWrappedJSToReleaseArray @ %x with %d wrappers(s)", \
                         &mWrappedJSToReleaseArray,
                         mWrappedJSToReleaseArray.Count()));

        XPC_LOG_ALWAYS(("mContextMap @ %x with %d context(s)", \
                         mContextMap, mContextMap ? mContextMap->Count() : 0));
        
        if(depth && mContextMap && mContextMap->Count())
        {
            XPC_LOG_INDENT();
            mContextMap->Enumerate(ContextMapDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }

        XPC_LOG_ALWAYS(("mWrappedJSClassMap @ %x with %d wrapperclasses(s)", \
                         mWrappedJSClassMap, mWrappedJSClassMap ? \
                                            mWrappedJSClassMap->Count() : 0));
        
        if(depth && mWrappedJSClassMap && mWrappedJSClassMap->Count())
        {
            XPC_LOG_INDENT();
            mWrappedJSClassMap->Enumerate(WrappedJSClassMapDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }
        XPC_LOG_ALWAYS(("mWrappedJSMap @ %x with %d wrappers(s)", \
                         mWrappedJSMap, mWrappedJSMap ? \
                                            mWrappedJSMap->Count() : 0));
        
        if(depth && mWrappedJSMap && mWrappedJSMap->Count())
        {
            XPC_LOG_INDENT();
            mWrappedJSMap->Enumerate(WrappedJSMapDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }

        XPC_LOG_ALWAYS(("mIID2NativeInterfaceMap @ %x with %d interface(s)", \
                         mIID2NativeInterfaceMap, mIID2NativeInterfaceMap ? \
                                    mIID2NativeInterfaceMap->Count() : 0));

        XPC_LOG_ALWAYS(("mClassInfo2NativeSetMap @ %x with %d sets(s)", \
                         mClassInfo2NativeSetMap, mClassInfo2NativeSetMap ? \
                                    mClassInfo2NativeSetMap->Count() : 0));

        XPC_LOG_ALWAYS(("mThisTranslatorMap @ %x with %d translator(s)", \
                         mThisTranslatorMap, mThisTranslatorMap ? \
                                    mThisTranslatorMap->Count() : 0));

        XPC_LOG_ALWAYS(("mNativeSetMap @ %x with %d sets(s)", \
                         mNativeSetMap, mNativeSetMap ? \
                                    mNativeSetMap->Count() : 0));

        
        if(depth && mNativeSetMap && mNativeSetMap->Count())
        {
            XPC_LOG_INDENT();
            mNativeSetMap->Enumerate(NativeSetDumpEnumerator, &depth);
            XPC_LOG_OUTDENT();
        }

        XPC_LOG_OUTDENT();
#endif
}



void
XPCRootSetElem::AddToRootSet(JSRuntime* rt, XPCRootSetElem** listHead)
{
    NS_ASSERTION(!mSelfp, "Must be not linked");
    JS_LOCK_GC(rt);
    mSelfp = listHead;
    mNext = *listHead;
    if(mNext)
    {
        NS_ASSERTION(mNext->mSelfp == listHead, "Must be list start");
        mNext->mSelfp = &mNext;
    }
    *listHead = this;
    JS_UNLOCK_GC(rt);
}

void
XPCRootSetElem::RemoveFromRootSet(JSRuntime* rt)
{
    NS_ASSERTION(mSelfp, "Must be linked");
    JS_LOCK_GC(rt);
    NS_ASSERTION(*mSelfp == this, "Link invariant");
    *mSelfp = mNext;
    if(mNext)
        mNext->mSelfp = mSelfp;
    JS_UNLOCK_GC(rt);
#ifdef DEBUG
    mSelfp = nsnull;
    mNext = nsnull;
#endif
}
