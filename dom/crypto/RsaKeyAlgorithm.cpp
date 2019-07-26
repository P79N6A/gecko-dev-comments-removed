





#include "mozilla/dom/RsaKeyAlgorithm.h"
#include "mozilla/dom/SubtleCryptoBinding.h"
#include "mozilla/dom/WebCryptoCommon.h"

namespace mozilla {
namespace dom {

JSObject*
RsaKeyAlgorithm::WrapObject(JSContext* aCx)
{
  return RsaKeyAlgorithmBinding::Wrap(aCx, this);
}

bool
RsaKeyAlgorithm::WriteStructuredClone(JSStructuredCloneWriter* aWriter) const
{
  return JS_WriteUint32Pair(aWriter, SCTAG_RSAKEYALG, 0) &&
         JS_WriteUint32Pair(aWriter, mModulusLength, 0) &&
         WriteBuffer(aWriter, mPublicExponent) &&
         WriteString(aWriter, mName);
}

KeyAlgorithm*
RsaKeyAlgorithm::Create(nsIGlobalObject* aGlobal, JSStructuredCloneReader* aReader)
{
  uint32_t modulusLength, zero;
  CryptoBuffer publicExponent;
  nsString name;
  bool read = JS_ReadUint32Pair(aReader, &modulusLength, &zero) &&
              ReadBuffer(aReader, publicExponent) &&
              ReadString(aReader, name);
  if (!read) {
    return nullptr;
  }

  return new RsaKeyAlgorithm(aGlobal, name, modulusLength, publicExponent);
}

} 
} 
