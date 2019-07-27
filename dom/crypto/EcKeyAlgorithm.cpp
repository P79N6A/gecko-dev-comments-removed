





#include "mozilla/dom/EcKeyAlgorithm.h"
#include "mozilla/dom/SubtleCryptoBinding.h"
#include "mozilla/dom/WebCryptoCommon.h"

namespace mozilla {
namespace dom {

JSObject*
EcKeyAlgorithm::WrapObject(JSContext* aCx)
{
  return EcKeyAlgorithmBinding::Wrap(aCx, this);
}

bool
EcKeyAlgorithm::WriteStructuredClone(JSStructuredCloneWriter* aWriter) const
{
  return JS_WriteUint32Pair(aWriter, SCTAG_ECKEYALG, 0) &&
         WriteString(aWriter, mNamedCurve) &&
         WriteString(aWriter, mName);
}

KeyAlgorithm*
EcKeyAlgorithm::Create(nsIGlobalObject* aGlobal, JSStructuredCloneReader* aReader)
{
  nsString name;
  nsString namedCurve;
  bool read = ReadString(aReader, namedCurve) &&
              ReadString(aReader, name);
  if (!read) {
    return nullptr;
  }

  return new EcKeyAlgorithm(aGlobal, name, namedCurve);
}

} 
} 
