




































#include "nsXPCOM.h"
#include "nsIGenericFactory.h"

#include "nsFileSpecImpl.h"

#define COMPONENT(NAME, Ctor)                                           \
 { NS_##NAME##_CLASSNAME, NS_##NAME##_CID, NS_##NAME##_CONTRACTID, Ctor }


static const nsModuleComponentInfo components[] =
{
  COMPONENT(FILESPEC, nsFileSpecImpl::Create),
  COMPONENT(DIRECTORYITERATOR, nsDirectoryIteratorImpl::Create),
};

NS_IMPL_NSGETMODULE(xpcomObsoleteModule, components)

