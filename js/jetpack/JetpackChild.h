




































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
                                           const nsTArray<Variant>& data);
  NS_OVERRIDE virtual bool RecvLoadImplementation(const nsCString& code);
  NS_OVERRIDE virtual bool RecvLoadUserScript(const nsCString& code);

  NS_OVERRIDE virtual PHandleChild* AllocPHandle();
  NS_OVERRIDE virtual bool DeallocPHandle(PHandleChild* actor);

private:
  JSRuntime* mRuntime;
  JSContext *mImplCx, *mUserCx;

  static JetpackChild* GetThis(JSContext* cx);

  static const JSPropertySpec sImplProperties[];
  static JSBool UserJetpackGetter(JSContext* cx, JSObject* obj, jsval idval,
                                  jsval* vp);

  static const JSFunctionSpec sImplMethods[];
  static JSBool SendMessage(JSContext* cx, uintN argc, jsval *vp);
  static JSBool CallMessage(JSContext* cx, uintN argc, jsval *vp);
  static JSBool RegisterReceiver(JSContext* cx, uintN argc, jsval *vp);
  static JSBool UnregisterReceiver(JSContext* cx, uintN argc, jsval *vp);
  static JSBool UnregisterReceivers(JSContext* cx, uintN argc, jsval *vp);
  static JSBool Wrap(JSContext* cx, uintN argc, jsval *vp);
  static JSBool CreateHandle(JSContext* cx, uintN argc, jsval *vp);

  static const JSClass sGlobalClass;

  DISALLOW_EVIL_CONSTRUCTORS(JetpackChild);
};

} 
} 


#endif 
