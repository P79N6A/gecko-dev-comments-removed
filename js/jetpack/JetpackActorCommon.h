





































#ifndef mozilla_jetpack_JetpackActorCommon_h
#define mozilla_jetpack_JetpackActorCommon_h

#include "nsClassHashtable.h"
#include "nsHashKeys.h"
#include "nsTArray.h"
#include "nsString.h"
#include "nsAutoJSValHolder.h"

struct JSContext;

namespace mozilla {
namespace jetpack {

class KeyValue;
class PrimVariant;
class CompVariant;
class Variant;

class JetpackActorCommon
{
public:

  bool
  RecvMessage(JSContext* cx,
              const nsString& messageName,
              const InfallibleTArray<Variant>& data,
              InfallibleTArray<Variant>* results);

  nsresult
  RegisterReceiver(JSContext* cx,
                   const nsString& messageName,
                   jsval receiver);

  void
  UnregisterReceiver(const nsString& messageName,
                     jsval receiver);

  void
  UnregisterReceivers(const nsString& messageName) {
    mReceivers.Remove(messageName);
  }

  void ClearReceivers() {
    mReceivers.Clear();
  }

  class OpaqueSeenType;
  static bool jsval_to_Variant(JSContext* cx, jsval from, Variant* to,
                               OpaqueSeenType* seen = NULL);
  static bool jsval_from_Variant(JSContext* cx, const Variant& from, jsval* to,
                                 OpaqueSeenType* seen = NULL);

protected:

  JetpackActorCommon() {
    mReceivers.Init();
    NS_ASSERTION(mReceivers.IsInitialized(),
                 "Failed to initialize message receiver hash set");
  }

private:

  static bool jsval_to_PrimVariant(JSContext* cx, JSType type, jsval from,
                                   PrimVariant* to);
  static bool jsval_to_CompVariant(JSContext* cx, JSType type, jsval from,
                                   CompVariant* to, OpaqueSeenType* seen);

  static bool jsval_from_PrimVariant(JSContext* cx, const PrimVariant& from,
                                     jsval* to);
  static bool jsval_from_CompVariant(JSContext* cx, const CompVariant& from,
                                     jsval* to, OpaqueSeenType* seen);

  
  
  class RecList
  {
    JSContext* mCx;
    class RecNode
    {
      nsAutoJSValHolder mHolder;
    public:
      RecNode* down;
      RecNode(JSContext* cx, jsval v) : down(NULL) {
        mHolder.Hold(cx);
        mHolder = v;
      }
      jsval value() { return mHolder; }
    }* mHead;
  public:
    RecList(JSContext* cx) : mCx(cx), mHead(NULL) {}
   ~RecList();
    void add(jsval v);
    void remove(jsval v);
    void copyTo(nsTArray<jsval>& dst) const;
  };

  nsClassHashtable<nsStringHashKey, RecList> mReceivers;

};

} 
} 

#endif
