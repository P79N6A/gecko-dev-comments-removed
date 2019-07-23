




































#ifndef nsCycleCollector_h__
#define nsCycleCollector_h__

class nsISupports;
class nsCycleCollectionParticipant;




struct nsCycleCollectionLanguageRuntime
{
    virtual nsresult BeginCycleCollection() = 0;
    virtual nsresult FinishCycleCollection() = 0;
    virtual nsCycleCollectionParticipant *ToParticipant(void *p) = 0;
};

NS_COM PRBool nsCycleCollector_suspect(nsISupports *n);
NS_COM void nsCycleCollector_suspectCurrent(nsISupports *n);
NS_COM PRBool nsCycleCollector_forget(nsISupports *n);
nsresult nsCycleCollector_startup();
NS_COM void nsCycleCollector_collect();
void nsCycleCollector_shutdown();

#ifdef DEBUG
NS_COM void nsCycleCollector_DEBUG_shouldBeFreed(nsISupports *n);
NS_COM void nsCycleCollector_DEBUG_wasFreed(nsISupports *n);
#endif



NS_COM void nsCycleCollector_registerRuntime(PRUint32 langID, nsCycleCollectionLanguageRuntime *rt);
NS_COM void nsCycleCollector_forgetRuntime(PRUint32 langID);

#endif 
