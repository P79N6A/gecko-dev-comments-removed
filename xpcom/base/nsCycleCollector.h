




#ifndef nsCycleCollector_h__
#define nsCycleCollector_h__

class nsICycleCollectorListener;
class nsISupports;

#include "nsError.h"
#include "nsID.h"

namespace mozilla {

class CycleCollectedJSRuntime;


typedef void* (*DeferredFinalizeAppendFunction)(void* pointers, void* thing);
typedef bool (*DeferredFinalizeFunction)(uint32_t slice, void* data);

}


class nsCycleCollectorResults
{
public:
    nsCycleCollectorResults() :
        mForcedGC(false), mMergedZones(false),
        mVisitedRefCounted(0), mVisitedGCed(0),
        mFreedRefCounted(0), mFreedGCed(0) {}
    bool mForcedGC;
    bool mMergedZones;
    uint32_t mVisitedRefCounted;
    uint32_t mVisitedGCed;
    uint32_t mFreedRefCounted;
    uint32_t mFreedGCed;
};

bool nsCycleCollector_init();

void nsCycleCollector_startup();

typedef void (*CC_BeforeUnlinkCallback)(void);
void nsCycleCollector_setBeforeUnlinkCallback(CC_BeforeUnlinkCallback aCB);

typedef void (*CC_ForgetSkippableCallback)(void);
void nsCycleCollector_setForgetSkippableCallback(CC_ForgetSkippableCallback aCB);

void nsCycleCollector_forgetSkippable(bool aRemoveChildlessNodes = false,
                                      bool aAsyncSnowWhiteFreeing = false);

void nsCycleCollector_dispatchDeferredDeletion(bool aContinuation = false);
bool nsCycleCollector_doDeferredDeletion();

void nsCycleCollector_collect(bool aManuallyTriggered,
                              nsCycleCollectorResults *aResults,
                              nsICycleCollectorListener *aManualListener);
uint32_t nsCycleCollector_suspectedCount();
void nsCycleCollector_shutdown();


void nsCycleCollector_registerJSRuntime(mozilla::CycleCollectedJSRuntime *aRt);
void nsCycleCollector_forgetJSRuntime();

#define NS_CYCLE_COLLECTOR_LOGGER_CID \
{ 0x58be81b4, 0x39d2, 0x437c, \
{ 0x94, 0xea, 0xae, 0xde, 0x2c, 0x62, 0x08, 0xd3 } }

extern nsresult
nsCycleCollectorLoggerConstructor(nsISupports* outer,
                                  const nsIID& aIID,
                                  void* *aInstancePtr);

namespace mozilla {
namespace cyclecollector {

#ifdef DEBUG
bool IsJSHolder(void* aHolder);
#endif

void DeferredFinalize(DeferredFinalizeAppendFunction aAppendFunc,
                      DeferredFinalizeFunction aFunc,
                      void* aThing);
void DeferredFinalize(nsISupports* aSupports);


} 
} 

#endif 
