




































#ifndef mozilla_GenericFactory_h
#define mozilla_GenericFactory_h

#include "mozilla/Module.h"

namespace mozilla {





class GenericFactory : public nsIFactory
{
public:
  typedef Module::ConstructorProc ConstructorProc;

  NS_DECL_ISUPPORTS
  NS_DECL_NSIFACTORY

  GenericFactory(ConstructorProc ctor)
    : mCtor(ctor)
  {
    NS_ASSERTION(mCtor, "GenericFactory with no constructor");
  }

private:
  ConstructorProc mCtor;
};

} 

#endif 
