





#include "mozilla/dom/CryptoKeyPair.h"
#include "mozilla/dom/SubtleCryptoBinding.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(CryptoKeyPair, mGlobal, mPublicKey, mPrivateKey)
NS_IMPL_CYCLE_COLLECTING_ADDREF(CryptoKeyPair)
NS_IMPL_CYCLE_COLLECTING_RELEASE(CryptoKeyPair)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(CryptoKeyPair)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
CryptoKeyPair::WrapObject(JSContext* aCx)
{
  return CryptoKeyPairBinding::Wrap(aCx, this);
}


} 
} 
