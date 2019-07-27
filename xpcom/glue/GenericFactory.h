





#ifndef mozilla_GenericFactory_h
#define mozilla_GenericFactory_h

#include "mozilla/Attributes.h"

#include "mozilla/Module.h"

namespace mozilla {






class GenericFactory final : public nsIFactory
{
  ~GenericFactory() {}

public:
  typedef Module::ConstructorProcPtr ConstructorProcPtr;

  NS_DECL_THREADSAFE_ISUPPORTS
  NS_DECL_NSIFACTORY

  explicit GenericFactory(ConstructorProcPtr aCtor)
    : mCtor(aCtor)
  {
    NS_ASSERTION(mCtor, "GenericFactory with no constructor");
  }

private:
  ConstructorProcPtr mCtor;
};

} 

#endif 
