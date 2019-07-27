





#ifndef TestCImplementedInterface_h
#define TestCImplementedInterface_h

#include "../TestJSImplGenBinding.h"

class nsPIDOMWindow;

namespace mozilla {
namespace dom {

class TestCImplementedInterface : public TestJSImplInterface
{
public:
  TestCImplementedInterface(JS::Handle<JSObject*> aJSImpl,
                            nsPIDOMWindow* aParent)
    : TestJSImplInterface(aJSImpl, aParent)
  {}
};

class TestCImplementedInterface2 : public nsISupports,
                                   public nsWrapperCache
{
public:
  explicit TestCImplementedInterface2(nsPIDOMWindow* aParent)
  {}
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TestCImplementedInterface2)

  
  nsISupports* GetParentObject();
};



} 
} 

#endif 
