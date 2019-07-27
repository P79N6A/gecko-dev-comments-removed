





#include "mozilla/dom/KeyAlgorithm.h"
#include "mozilla/dom/WebCryptoCommon.h"
#include "mozilla/dom/AesKeyAlgorithm.h"
#include "mozilla/dom/EcKeyAlgorithm.h"
#include "mozilla/dom/HmacKeyAlgorithm.h"
#include "mozilla/dom/RsaKeyAlgorithm.h"
#include "mozilla/dom/RsaHashedKeyAlgorithm.h"
#include "mozilla/dom/SubtleCryptoBinding.h"
#include "mozilla/dom/WebCryptoCommon.h"

namespace mozilla {
namespace dom {


NS_IMPL_CYCLE_COLLECTION_WRAPPERCACHE(KeyAlgorithm, mGlobal)
NS_IMPL_CYCLE_COLLECTING_ADDREF(KeyAlgorithm)
NS_IMPL_CYCLE_COLLECTING_RELEASE(KeyAlgorithm)
NS_INTERFACE_MAP_BEGIN_CYCLE_COLLECTION(KeyAlgorithm)
  NS_WRAPPERCACHE_INTERFACE_MAP_ENTRY
  NS_INTERFACE_MAP_ENTRY(nsISupports)
NS_INTERFACE_MAP_END

KeyAlgorithm::KeyAlgorithm(nsIGlobalObject* aGlobal, const nsString& aName)
  : mGlobal(aGlobal)
  , mName(aName)
{
  SetIsDOMBinding();

  
  mMechanism = MapAlgorithmNameToMechanism(aName);

  
}

KeyAlgorithm::~KeyAlgorithm()
{
}

JSObject*
KeyAlgorithm::WrapObject(JSContext* aCx)
{
  return KeyAlgorithmBinding::Wrap(aCx, this);
}

nsString
KeyAlgorithm::ToJwkAlg() const
{
  return nsString();
}

void
KeyAlgorithm::GetName(nsString& aRetVal) const
{
  aRetVal.Assign(mName);
}

bool
KeyAlgorithm::WriteStructuredClone(JSStructuredCloneWriter* aWriter) const
{
  return WriteString(aWriter, mName);
}

KeyAlgorithm*
KeyAlgorithm::Create(nsIGlobalObject* aGlobal, JSStructuredCloneReader* aReader)
{
  uint32_t tag, zero;
  bool read = JS_ReadUint32Pair( aReader, &tag, &zero );
  if (!read) {
    return nullptr;
  }

  KeyAlgorithm* algorithm = nullptr;
  switch (tag) {
    case SCTAG_KEYALG: {
      nsString name;
      read = ReadString(aReader, name);
      if (!read) {
        return nullptr;
      }
      algorithm = new KeyAlgorithm(aGlobal, name);
      break;
    }
    case SCTAG_AESKEYALG: {
      algorithm = AesKeyAlgorithm::Create(aGlobal, aReader);
      break;
    }
    case SCTAG_ECKEYALG: {
      algorithm = EcKeyAlgorithm::Create(aGlobal, aReader);
      break;
    }
    case SCTAG_HMACKEYALG: {
      algorithm = HmacKeyAlgorithm::Create(aGlobal, aReader);
      break;
    }
    case SCTAG_RSAKEYALG: {
      algorithm = RsaKeyAlgorithm::Create(aGlobal, aReader);
      break;
    }
    case SCTAG_RSAHASHEDKEYALG: {
      algorithm = RsaHashedKeyAlgorithm::Create(aGlobal, aReader);
      break;
    }
    
  }

  return algorithm;
}

} 
} 
