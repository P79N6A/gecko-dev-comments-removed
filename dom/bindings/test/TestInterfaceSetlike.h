





#ifndef mozilla_dom_TestInterfaceSetlike_h
#define mozilla_dom_TestInterfaceSetlike_h

#include "nsWrapperCache.h"
#include "nsCOMPtr.h"

class nsPIDOMWindow;

namespace mozilla {

class ErrorResult;

namespace dom {

class GlobalObject;



class TestInterfaceSetlike final : public nsISupports,
                                   public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TestInterfaceSetlike)
  explicit TestInterfaceSetlike(JSContext* aCx,
                                nsPIDOMWindow* aParent);
  nsPIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;
  static already_AddRefed<TestInterfaceSetlike>
    Constructor(const GlobalObject& aGlobal, ErrorResult& rv);
private:
  virtual ~TestInterfaceSetlike() {}
  nsCOMPtr<nsPIDOMWindow> mParent;
};

} 
} 

#endif 
