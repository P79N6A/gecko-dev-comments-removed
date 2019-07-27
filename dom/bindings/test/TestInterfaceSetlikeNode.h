





#ifndef mozilla_dom_TestInterfaceSetlikeNode_h
#define mozilla_dom_TestInterfaceSetlikeNode_h

#include "nsWrapperCache.h"
#include "nsCOMPtr.h"

class nsPIDOMWindow;

namespace mozilla {

class ErrorResult;

namespace dom {

class GlobalObject;



class TestInterfaceSetlikeNode final : public nsISupports,
                                       public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TestInterfaceSetlikeNode)
  explicit TestInterfaceSetlikeNode(JSContext* aCx,
                                    nsPIDOMWindow* aParent);
  nsPIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;
  static already_AddRefed<TestInterfaceSetlikeNode>
    Constructor(const GlobalObject& aGlobal, ErrorResult& rv);
private:
  virtual ~TestInterfaceSetlikeNode() {}
  nsCOMPtr<nsPIDOMWindow> mParent;
};

} 
} 

#endif 
