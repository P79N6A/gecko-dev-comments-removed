




































#ifndef nsCycleCollector_h__
#define nsCycleCollector_h__





class nsISupports;
class nsCycleCollectionParticipant;
class nsCycleCollectionTraversalCallback;




struct nsCycleCollectionLanguageRuntime
{
    virtual nsresult BeginCycleCollection(nsCycleCollectionTraversalCallback &cb) = 0;
    virtual nsresult FinishCycleCollection() = 0;
    virtual nsCycleCollectionParticipant *ToParticipant(void *p) = 0;
    virtual void CommenceShutdown() = 0;
#ifdef DEBUG_CC
    virtual void PrintAllReferencesTo(void *p) = 0;
#endif
};

nsresult nsCycleCollector_startup();

NS_COM PRUint32 nsCycleCollector_collect();
NS_COM PRUint32 nsCycleCollector_suspectedCount();
void nsCycleCollector_shutdown();






struct nsCycleCollectionJSRuntime : public nsCycleCollectionLanguageRuntime
{
    



    virtual PRBool Collect() = 0;
};

NS_COM PRBool nsCycleCollector_beginCollection();


NS_COM PRBool nsCycleCollector_finishCollection();

#ifdef DEBUG
NS_COM void nsCycleCollector_DEBUG_shouldBeFreed(nsISupports *n);
NS_COM void nsCycleCollector_DEBUG_wasFreed(nsISupports *n);
#endif



NS_COM void nsCycleCollector_registerRuntime(PRUint32 langID, nsCycleCollectionLanguageRuntime *rt);
NS_COM nsCycleCollectionLanguageRuntime * nsCycleCollector_getRuntime(PRUint32 langID);
NS_COM void nsCycleCollector_forgetRuntime(PRUint32 langID);

#endif 
