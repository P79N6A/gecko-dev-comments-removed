




































#include "base/basictypes.h"
#include "mozilla/jetpack/JetpackService.h"

#include "mozilla/jetpack/JetpackParent.h"
#include "nsIJetpack.h"

#include "nsIGenericFactory.h"

namespace mozilla {
namespace jetpack {

NS_IMPL_ISUPPORTS1(JetpackService,
                   nsIJetpackService)

NS_IMETHODIMP
JetpackService::CreateJetpack(nsIJetpack** aResult)
{
  nsRefPtr<JetpackParent> j = new JetpackParent();
  *aResult = j.forget().get();
  return NS_OK;
}

NS_GENERIC_FACTORY_CONSTRUCTOR(JetpackService)

} 
} 

#define JETPACKSERVICE_CID \
{ 0x4cf18fcd, 0x4247, 0x4388, \
  { 0xb1, 0x88, 0xb0, 0x72, 0x2a, 0xc0, 0x52, 0x21 } }

static const nsModuleComponentInfo kComponents[] = {
  {
    "mozilla::jetpack::JetpackService",
    JETPACKSERVICE_CID,
    "@mozilla.org/jetpack/service;1",
    mozilla::jetpack::JetpackServiceConstructor
  }
};

NS_IMPL_NSGETMODULE(jetpack, kComponents)

