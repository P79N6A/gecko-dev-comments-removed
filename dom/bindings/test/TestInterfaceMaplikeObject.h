





#ifndef mozilla_dom_TestInterfaceMaplikeObject_h
#define mozilla_dom_TestInterfaceMaplikeObject_h

#include "nsWrapperCache.h"
#include "nsCOMPtr.h"

class nsPIDOMWindow;

namespace mozilla {

class ErrorResult;

namespace dom {

class GlobalObject;



class TestInterfaceMaplikeObject final : public nsISupports,
                                         public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TestInterfaceMaplikeObject)

  explicit TestInterfaceMaplikeObject(nsPIDOMWindow* aParent);
  nsPIDOMWindow* GetParentObject() const;
  virtual JSObject* WrapObject(JSContext* aCx,
                               JS::Handle<JSObject*> aGivenProto) override;
  static already_AddRefed<TestInterfaceMaplikeObject>
    Constructor(const GlobalObject& aGlobal,ErrorResult& rv);

  
  void SetInternal(const nsAString& aKey);
  void ClearInternal();
  bool DeleteInternal(const nsAString& aKey);
  bool HasInternal(const nsAString& aKey);
private:
  virtual ~TestInterfaceMaplikeObject() {}
  nsCOMPtr<nsPIDOMWindow> mParent;
};

} 
} 

#endif 
