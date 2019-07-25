




































#ifndef nsCycleCollector_h__
#define nsCycleCollector_h__





class nsISupports;
class nsICycleCollectorListener;
class nsCycleCollectionParticipant;
class nsCycleCollectionTraversalCallback;




struct nsCycleCollectionLanguageRuntime
{
    virtual nsresult BeginCycleCollection(nsCycleCollectionTraversalCallback &cb,
                                          bool explainLiveExpectedGarbage) = 0;
    virtual nsresult FinishTraverse() = 0;
    virtual nsresult FinishCycleCollection() = 0;
    virtual nsCycleCollectionParticipant *ToParticipant(void *p) = 0;
#ifdef DEBUG_CC
    virtual void PrintAllReferencesTo(void *p) = 0;
#endif
};

nsresult nsCycleCollector_startup();

PRUint32 nsCycleCollector_collect(nsICycleCollectorListener *aListener);
PRUint32 nsCycleCollector_suspectedCount();
void nsCycleCollector_shutdownThreads();
void nsCycleCollector_shutdown();






struct nsCycleCollectionJSRuntime : public nsCycleCollectionLanguageRuntime
{
    


    virtual void NotifyLeaveMainThread() = 0;
    virtual void NotifyEnterCycleCollectionThread() = 0;
    virtual void NotifyLeaveCycleCollectionThread() = 0;
    virtual void NotifyEnterMainThread() = 0;

    


    virtual bool NeedCollect() = 0;

    


    virtual void Collect(bool shrinkingGC = false) = 0;
};

#ifdef DEBUG
void nsCycleCollector_DEBUG_shouldBeFreed(nsISupports *n);
void nsCycleCollector_DEBUG_wasFreed(nsISupports *n);
#endif



void nsCycleCollector_registerRuntime(PRUint32 langID, nsCycleCollectionLanguageRuntime *rt);
nsCycleCollectionLanguageRuntime * nsCycleCollector_getRuntime(PRUint32 langID);
void nsCycleCollector_forgetRuntime(PRUint32 langID);

#define NS_CYCLE_COLLECTOR_LOGGER_CID \
{ 0x58be81b4, 0x39d2, 0x437c, \
{ 0x94, 0xea, 0xae, 0xde, 0x2c, 0x62, 0x08, 0xd3 } }

extern nsresult
nsCycleCollectorLoggerConstructor(nsISupports* outer,
                                  const nsIID& aIID,
                                  void* *aInstancePtr);

#endif 
