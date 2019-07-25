





































#ifndef mozilla_jetpack_JetpackParent_h
#define mozilla_jetpack_JetpackParent_h

#include "mozilla/jetpack/PJetpackParent.h"
#include "mozilla/jetpack/JetpackProcessParent.h"
#include "mozilla/jetpack/JetpackActorCommon.h"
#include "nsIJetpack.h"

#include "nsTArray.h"

struct JSContext;

namespace mozilla {
namespace jetpack {

class PHandleParent;

class JetpackParent
  : public PJetpackParent
  , public nsIJetpack
  , private JetpackActorCommon
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIJETPACK

  JetpackParent(JSContext* cx);
  ~JetpackParent();

  void OnChannelConnected(int32 pid);

protected:
  NS_OVERRIDE virtual bool RecvSendMessage(const nsString& messageName,
                                           const InfallibleTArray<Variant>& data);
  NS_OVERRIDE virtual bool AnswerCallMessage(const nsString& messageName,
                                             const InfallibleTArray<Variant>& data,
                                             InfallibleTArray<Variant>* results);

  NS_OVERRIDE virtual PHandleParent* AllocPHandle();
  NS_OVERRIDE virtual bool DeallocPHandle(PHandleParent* actor);

private:
  JetpackProcessParent* mSubprocess;
  JSContext* mContext;

  DISALLOW_EVIL_CONSTRUCTORS(JetpackParent);
};

} 
} 

#endif 
