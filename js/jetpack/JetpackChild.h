




































#ifndef mozilla_jetpack_JetpackChild_h
#define mozilla_jetpack_JetpackChild_h

#include "mozilla/jetpack/PJetpackChild.h"
#include "mozilla/jetpack/JetpackActorCommon.h"

#include "nsTArray.h"

namespace mozilla {
namespace jetpack {

class PHandleChild;

class JetpackChild
  : public PJetpackChild
  , private JetpackActorCommon
{
public:
  JetpackChild();
  ~JetpackChild();

  static JetpackChild* current();

  bool Init(base::ProcessHandle aParentProcessHandle,
            MessageLoop* aIOLoop,
            IPC::Channel* aChannel);

  void CleanUp();

protected:
  NS_OVERRIDE virtual void ActorDestroy(ActorDestroyReason why);

  NS_OVERRIDE virtual bool RecvSendMessage(const nsString& messageName,
                                           const InfallibleTArray<Variant>& data);
  NS_OVERRIDE virtual bool RecvEvalScript(const nsString& script);

  NS_OVERRIDE virtual PHandleChild* AllocPHandle();
  NS_OVERRIDE virtual bool DeallocPHandle(PHandleChild* actor);

private:
  JSRuntime* mRuntime;
  JSContext *mCx;

  static JetpackChild* GetThis(JSContext* cx);

  static const JSFunctionSpec sImplMethods[];
  static JSBool SendMessage(JSContext* cx, uintN argc, jsval *vp);
  static JSBool CallMessage(JSContext* cx, uintN argc, jsval *vp);
  static JSBool RegisterReceiver(JSContext* cx, uintN argc, jsval *vp);
  static JSBool UnregisterReceiver(JSContext* cx, uintN argc, jsval *vp);
  static JSBool UnregisterReceivers(JSContext* cx, uintN argc, jsval *vp);
  static JSBool CreateHandle(JSContext* cx, uintN argc, jsval *vp);
  static JSBool CreateSandbox(JSContext* cx, uintN argc, jsval *vp);
  static JSBool EvalInSandbox(JSContext* cx, uintN argc, jsval *vp);
  static JSBool GC(JSContext* cx, uintN argc, jsval *vp);
#ifdef JS_GC_ZEAL
  static JSBool GCZeal(JSContext* cx, uintN argc, jsval *vp);
#endif

  static void ReportError(JSContext* cx, const char* message,
                          JSErrorReport* report);

  static const JSClass sGlobalClass;
  static bool sReportingError;

  DISALLOW_EVIL_CONSTRUCTORS(JetpackChild);
};

} 
} 


#endif 
