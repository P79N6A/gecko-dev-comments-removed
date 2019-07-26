





#include "mozilla/dom/HmacKeyAlgorithm.h"
#include "mozilla/dom/SubtleCryptoBinding.h"

namespace mozilla {
namespace dom {

NS_IMPL_CYCLE_COLLECTION_INHERITED(HmacKeyAlgorithm, KeyAlgorithm, mHash)
NS_IMPL_ADDREF_INHERITED(HmacKeyAlgorithm, KeyAlgorithm)
NS_IMPL_RELEASE_INHERITED(HmacKeyAlgorithm, KeyAlgorithm)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(HmacKeyAlgorithm)
NS_INTERFACE_MAP_END_INHERITING(KeyAlgorithm)

JSObject*
HmacKeyAlgorithm::WrapObject(JSContext* aCx)
{
  return HmacKeyAlgorithmBinding::Wrap(aCx, this);
}

bool
HmacKeyAlgorithm::WriteStructuredClone(JSStructuredCloneWriter* aWriter) const
{
  nsString hashName;
  mHash->GetName(hashName);
  return JS_WriteUint32Pair(aWriter, SCTAG_HMACKEYALG, 0) &&
         JS_WriteUint32Pair(aWriter, mLength, 0) &&
         WriteString(aWriter, hashName) &&
         WriteString(aWriter, mName);
}

KeyAlgorithm*
HmacKeyAlgorithm::Create(nsIGlobalObject* aGlobal, JSStructuredCloneReader* aReader)
{
  uint32_t length, zero;
  nsString hash, name;
  bool read = JS_ReadUint32Pair(aReader, &length, &zero) &&
              ReadString(aReader, hash) &&
              ReadString(aReader, name);
  if (!read) {
    return nullptr;
  }

  return new HmacKeyAlgorithm(aGlobal, name, length, hash);
}

} 
} 
