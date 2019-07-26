




#ifndef nsCycleCollector_h__
#define nsCycleCollector_h__

class nsICycleCollectorListener;
class nsICycleCollectorLogSink;
class nsISupports;
template<class T> struct already_AddRefed;

#include "nsError.h"
#include "nsID.h"

namespace mozilla {

class CycleCollectedJSRuntime;


typedef void* (*DeferredFinalizeAppendFunction)(void* pointers, void* thing);
typedef bool (*DeferredFinalizeFunction)(uint32_t slice, void* data);

}

bool nsCycleCollector_init();

void nsCycleCollector_startup();

typedef void (*CC_BeforeUnlinkCallback)(void);
void nsCycleCollector_setBeforeUnlinkCallback(CC_BeforeUnlinkCallback aCB);

typedef void (*CC_ForgetSkippableCallback)(void);
void nsCycleCollector_setForgetSkippableCallback(CC_ForgetSkippableCallback aCB);

void nsCycleCollector_forgetSkippable(bool aRemoveChildlessNodes = false,
                                      bool aAsyncSnowWhiteFreeing = false);

void nsCycleCollector_prepareForGarbageCollection();


void nsCycleCollector_finishAnyCurrentCollection();

void nsCycleCollector_dispatchDeferredDeletion(bool aContinuation = false);
bool nsCycleCollector_doDeferredDeletion();

already_AddRefed<nsICycleCollectorLogSink> nsCycleCollector_createLogSink();

void nsCycleCollector_collect(nsICycleCollectorListener* aManualListener);



void nsCycleCollector_collectSlice(int64_t aSliceTime);



void nsCycleCollector_collectSliceWork(int64_t aSliceWork);

uint32_t nsCycleCollector_suspectedCount();
void nsCycleCollector_shutdown();


void nsCycleCollector_registerJSRuntime(mozilla::CycleCollectedJSRuntime* aRt);
void nsCycleCollector_forgetJSRuntime();

#define NS_CYCLE_COLLECTOR_LOGGER_CID \
{ 0x58be81b4, 0x39d2, 0x437c, \
{ 0x94, 0xea, 0xae, 0xde, 0x2c, 0x62, 0x08, 0xd3 } }

extern nsresult
nsCycleCollectorLoggerConstructor(nsISupports* aOuter,
                                  const nsIID& aIID,
                                  void** aInstancePtr);

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
