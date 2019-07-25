




































#include "base/basictypes.h"
#include "mozilla/jetpack/JetpackService.h"

#include "mozilla/jetpack/JetpackParent.h"
#include "nsIJetpack.h"

#include "mozilla/ModuleUtils.h"

#include "nsIXPConnect.h"

namespace mozilla {
namespace jetpack {

NS_IMPL_ISUPPORTS1(JetpackService,
                   nsIJetpackService)

NS_IMETHODIMP
JetpackService::CreateJetpack(nsIJetpack** aResult)
{
  nsresult rv;
  nsCOMPtr<nsIXPConnect> xpc(do_GetService(nsIXPConnect::GetCID(), &rv));
  NS_ENSURE_SUCCESS(rv, rv);

  nsAXPCNativeCallContext* ncc = NULL;
  rv = xpc->GetCurrentNativeCallContext(&ncc);
  NS_ENSURE_SUCCESS(rv, rv);

  JSContext* cx;
  rv = ncc->GetJSContext(&cx);
  NS_ENSURE_SUCCESS(rv, rv);

  nsRefPtr<JetpackParent> j = new JetpackParent(cx);
  *aResult = j.forget().get();

  return NS_OK;
}

NS_GENERIC_FACTORY_CONSTRUCTOR(JetpackService)

} 
} 

#define JETPACKSERVICE_CID \
{ 0x4cf18fcd, 0x4247, 0x4388, \
  { 0xb1, 0x88, 0xb0, 0x72, 0x2a, 0xc0, 0x52, 0x21 } }

NS_DEFINE_NAMED_CID(JETPACKSERVICE_CID);

static const mozilla::Module::CIDEntry kJetpackCIDs[] = {
  { &kJETPACKSERVICE_CID, false, NULL, mozilla::jetpack::JetpackServiceConstructor },
  { NULL }
};

static const mozilla::Module::ContractIDEntry kJetpackContracts[] = {
  { "@mozilla.org/jetpack/service;1", &kJETPACKSERVICE_CID },
  { NULL }
};

static const mozilla::Module kJetpackModule = {
  mozilla::Module::kVersion,
  kJetpackCIDs,
  kJetpackContracts
};

NSMODULE_DEFN(jetpack) = &kJetpackModule;

