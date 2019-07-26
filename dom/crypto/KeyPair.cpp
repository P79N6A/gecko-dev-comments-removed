





#include "mozilla/dom/KeyPair.h"
#include "mozilla/dom/SubtleCryptoBinding.h"
#include "nsContentUtils.h"

namespace mozilla {
namespace dom {


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(KeyPair, mGlobal, mPublicKey, mPrivateKey)
NS_IMPL_CYCLE_COLLECTING_ADDREF(KeyPair)
NS_IMPL_CYCLE_COLLECTING_RELEASE(KeyPair)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(KeyPair)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

JSObject*
KeyPair::WrapObject(JSContext* aCx)
{
  return KeyPairBinding::Wrap(aCx, this);
}


} 
} 
