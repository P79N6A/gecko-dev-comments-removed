



#include "FinalizationWitnessService.h"

#include "nsString.h"
#include "jsapi.h"
#include "js/CallNonGenericMethod.h"
#include "mozJSComponentLoader.h"
#include "nsZipArchive.h"

#include "mozilla/Scoped.h"
#include "mozilla/Services.h"
#include "nsIObserverService.h"
#include "nsThreadUtils.h"




namespace mozilla {

namespace {









class FinalizationEvent final: public nsRunnable
{
public:
  FinalizationEvent(const char* aTopic,
                  const char16_t* aValue)
    : mTopic(aTopic)
    , mValue(aValue)
  { }

  NS_METHOD Run() {
    nsCOMPtr<nsIObserverService> observerService =
      mozilla::services::GetObserverService();
    if (!observerService) {
      
      
      return NS_ERROR_NOT_AVAILABLE;
    }
    (void)observerService->
      NotifyObservers(nullptr, mTopic.get(), mValue.get());
    return NS_OK;
  }
private:
  




  const nsCString mTopic;

  




  const nsString mValue;
};

enum {
  WITNESS_SLOT_EVENT,
  WITNESS_INSTANCES_SLOTS
};





already_AddRefed<FinalizationEvent>
ExtractFinalizationEvent(JSObject *objSelf)
{
  JS::Value slotEvent = JS_GetReservedSlot(objSelf, WITNESS_SLOT_EVENT);
  if (slotEvent.isUndefined()) {
    
    return nullptr;
  }

  JS_SetReservedSlot(objSelf, WITNESS_SLOT_EVENT, JS::UndefinedValue());

  return dont_AddRef(static_cast<FinalizationEvent*>(slotEvent.toPrivate()));
}







void Finalize(JSFreeOp *fop, JSObject *objSelf)
{
  nsRefPtr<FinalizationEvent> event = ExtractFinalizationEvent(objSelf);
  if (event == nullptr) {
    
    return;
  }

  
  
  (void)NS_DispatchToMainThread(event);
  
  
}

static const JSClass sWitnessClass = {
  "FinalizationWitness",
  JSCLASS_HAS_RESERVED_SLOTS(WITNESS_INSTANCES_SLOTS),
  nullptr ,
  nullptr ,
  nullptr ,
  nullptr ,
  nullptr ,
  nullptr ,
  nullptr ,
  nullptr ,
  Finalize 
};

bool IsWitness(JS::Handle<JS::Value> v)
{
  return v.isObject() && JS_GetClass(&v.toObject()) == &sWitnessClass;
}










bool ForgetImpl(JSContext* cx, JS::CallArgs args)
{
  if (args.length() != 0) {
    JS_ReportError(cx, "forget() takes no arguments");
    return false;
  }
  JS::Rooted<JS::Value> valSelf(cx, args.thisv());
  JS::Rooted<JSObject*> objSelf(cx, &valSelf.toObject());

  nsRefPtr<FinalizationEvent> event = ExtractFinalizationEvent(objSelf);
  if (event == nullptr) {
    JS_ReportError(cx, "forget() called twice");
    return false;
  }

  args.rval().setUndefined();
  return true;
}

bool Forget(JSContext *cx, unsigned argc, JS::Value *vp)
{
  JS::CallArgs args = CallArgsFromVp(argc, vp);
  return JS::CallNonGenericMethod<IsWitness, ForgetImpl>(cx, args);
}

static const JSFunctionSpec sWitnessClassFunctions[] = {
  JS_FN("forget", Forget, 0, JSPROP_READONLY | JSPROP_PERMANENT),
  JS_FS_END
};

}

NS_IMPL_ISUPPORTS(FinalizationWitnessService, nsIFinalizationWitnessService)













NS_IMETHODIMP
FinalizationWitnessService::Make(const char* aTopic,
                                 const char16_t* aValue,
                                 JSContext* aCx,
                                 JS::MutableHandle<JS::Value> aRetval)
{
  JS::Rooted<JSObject*> objResult(aCx, JS_NewObject(aCx, &sWitnessClass));
  if (!objResult) {
    return NS_ERROR_OUT_OF_MEMORY;
  }
  if (!JS_DefineFunctions(aCx, objResult, sWitnessClassFunctions)) {
    return NS_ERROR_FAILURE;
  }

  nsRefPtr<FinalizationEvent> event = new FinalizationEvent(aTopic, aValue);

  
  JS_SetReservedSlot(objResult, WITNESS_SLOT_EVENT,
                     JS::PrivateValue(event.forget().take()));

  aRetval.setObject(*objResult);
  return NS_OK;
}

} 
