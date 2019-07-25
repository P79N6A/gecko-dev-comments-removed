




































#ifndef mozilla_GenericFactory_h
#define mozilla_GenericFactory_h

#include "mozilla/Attributes.h"

#include "mozilla/Module.h"

namespace mozilla {






class GenericFactory MOZ_FINAL : public nsIFactory
{
public:
  typedef Module::ConstructorProcPtr ConstructorProcPtr;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIFACTORY

  GenericFactory(ConstructorProcPtr ctor)
    : mCtor(ctor)
  {
    NS_ASSERTION(mCtor, "GenericFactory with no constructor");
  }

private:
  ConstructorProcPtr mCtor;
};

} 

#endif 
