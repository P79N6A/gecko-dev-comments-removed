





#include "mozilla/dom/AesKeyAlgorithm.h"
#include "mozilla/dom/SubtleCryptoBinding.h"
#include "mozilla/dom/WebCryptoCommon.h"

namespace mozilla {
namespace dom {

JSObject*
AesKeyAlgorithm::WrapObject(JSContext* aCx)
{
  return AesKeyAlgorithmBinding::Wrap(aCx, this);
}

bool
AesKeyAlgorithm::WriteStructuredClone(JSStructuredCloneWriter* aWriter) const
{
  return JS_WriteUint32Pair(aWriter, SCTAG_AESKEYALG, 0) &&
         JS_WriteUint32Pair(aWriter, mLength, 0) &&
         WriteString(aWriter, mName);
}

KeyAlgorithm*
AesKeyAlgorithm::Create(nsIGlobalObject* aGlobal, JSStructuredCloneReader* aReader)
{
  uint32_t length, zero;
  nsString name;
  bool read = JS_ReadUint32Pair(aReader, &length, &zero) &&
              ReadString(aReader, name);
  if (!read) {
    return nullptr;
  }

  return new AesKeyAlgorithm(aGlobal, name, length);
}


} 
} 
