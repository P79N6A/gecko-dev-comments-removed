




#ifndef nsCycleCollector_h__
#define nsCycleCollector_h__

class nsCycleCollectionJSRuntime;
class nsICycleCollectorListener;
class nsISupports;


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

bool nsCycleCollector_init();

enum CCThreadingModel {
    CCSingleThread,
    CCWithTraverseThread
};

nsresult nsCycleCollector_startup(CCThreadingModel aThreadingModel);

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


void nsCycleCollector_registerJSRuntime(nsCycleCollectionJSRuntime *aRt);
void nsCycleCollector_forgetJSRuntime();

#define NS_CYCLE_COLLECTOR_LOGGER_CID \
{ 0x58be81b4, 0x39d2, 0x437c, \
{ 0x94, 0xea, 0xae, 0xde, 0x2c, 0x62, 0x08, 0xd3 } }

extern nsresult
nsCycleCollectorLoggerConstructor(nsISupports* outer,
                                  const nsIID& aIID,
                                  void* *aInstancePtr);

#endif 
