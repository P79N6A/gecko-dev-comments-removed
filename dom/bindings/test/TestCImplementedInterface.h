





#ifndef TestCImplementedInterface_h
#define TestCImplementedInterface_h

#include "../TestJSImplGenBinding.h"

namespace mozilla {
namespace dom {

class TestCImplementedInterface : public TestJSImplInterface
{
public:
  TestCImplementedInterface(JSObject* aJSImpl, nsISupports* aParent)
    : TestJSImplInterface(aJSImpl, aParent)
  {}
};

class TestCImplementedInterface2 : public nsISupports,
                                   public nsWrapperCache
{
public:
  NS_DECL_CYCLE_COLLECTING_ISUPPORTS
  NS_DECL_CYCLE_COLLECTION_SCRIPT_HOLDER_CLASS(TestCImplementedInterface2)

  
  nsISupports* GetParentObject();
};



} 
} 

#endif 
