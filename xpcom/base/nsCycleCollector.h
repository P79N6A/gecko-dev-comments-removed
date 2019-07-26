




#ifndef nsCycleCollector_h__
#define nsCycleCollector_h__

class nsISupports;
class nsICycleCollectorListener;
class nsCycleCollectionParticipant;
class nsCycleCollectionTraversalCallback;


class nsCycleCollectorResults
{
public:
    nsCycleCollectorResults() :
        mForcedGC(false), mVisitedRefCounted(0), mVisitedGCed(0),
        mFreedRefCounted(0), mFreedGCed(0) {}
    bool mForcedGC;
    uint32_t mVisitedRefCounted;
    uint32_t mVisitedGCed;
    uint32_t mFreedRefCounted;
    uint32_t mFreedGCed;
};

nsresult nsCycleCollector_startup();

typedef void (*CC_BeforeUnlinkCallback)(void);
void nsCycleCollector_setBeforeUnlinkCallback(CC_BeforeUnlinkCallback aCB);

typedef void (*CC_ForgetSkippableCallback)(void);
void nsCycleCollector_setForgetSkippableCallback(CC_ForgetSkippableCallback aCB);

void nsCycleCollector_forgetSkippable(bool aRemoveChildlessNodes = false);

void nsCycleCollector_collect(bool aMergeCompartments,
                              nsCycleCollectorResults *aResults,
                              nsICycleCollectorListener *aListener);
uint32_t nsCycleCollector_suspectedCount();
void nsCycleCollector_shutdownThreads();
void nsCycleCollector_shutdown();


struct nsCycleCollectionJSRuntime
{
    virtual nsresult BeginCycleCollection(nsCycleCollectionTraversalCallback &cb) = 0;
    virtual nsresult FinishTraverse() = 0;

    





    virtual bool NotifyLeaveMainThread() = 0;
    virtual void NotifyEnterCycleCollectionThread() = 0;
    virtual void NotifyLeaveCycleCollectionThread() = 0;
    virtual void NotifyEnterMainThread() = 0;

    


    virtual void FixWeakMappingGrayBits() = 0;

    


    virtual bool NeedCollect() = 0;

    


    virtual void Collect(uint32_t reason) = 0;

    


    virtual nsCycleCollectionParticipant *GetParticipant() = 0;

#ifdef DEBUG
    virtual void SetObjectToUnlink(void* aObject) = 0;
    virtual void AssertNoObjectsToTrace(void* aPossibleJSHolder) = 0;
#endif
};


void nsCycleCollector_registerJSRuntime(nsCycleCollectionJSRuntime *rt);
void nsCycleCollector_forgetJSRuntime();

#define NS_CYCLE_COLLECTOR_LOGGER_CID \
{ 0x58be81b4, 0x39d2, 0x437c, \
{ 0x94, 0xea, 0xae, 0xde, 0x2c, 0x62, 0x08, 0xd3 } }

extern nsresult
nsCycleCollectorLoggerConstructor(nsISupports* outer,
                                  const nsIID& aIID,
                                  void* *aInstancePtr);

#endif 
