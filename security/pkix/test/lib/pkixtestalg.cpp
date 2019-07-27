























#include "pkixtestutil.h"

#include "pkixder.h"


#define PREFIX_1_2_840_10040 0x2a, 0x86, 0x48, 0xce, 0x38


#define PREFIX_1_2_840_10045 0x2a, 0x86, 0x48, 0xce, 0x3d


#define PREFIX_1_2_840_113549 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d

namespace mozilla { namespace pkix { namespace test {

namespace {

enum class NULLParam { NO, YES };

template <size_t SIZE>
ByteString
OID(const uint8_t (&rawValue)[SIZE])
{
  return TLV(der::OIDTag, ByteString(rawValue, SIZE));
}

template <size_t SIZE>
ByteString
SimpleAlgID(const uint8_t (&rawValue)[SIZE],
            NULLParam nullParam = NULLParam::NO)
{
  ByteString sequenceValue(OID(rawValue));
  if (nullParam == NULLParam::YES) {
    sequenceValue.append(TLV(der::NULLTag, ByteString()));
  }
  return TLV(der::SEQUENCE, sequenceValue);
}

} 

TestSignatureAlgorithm::TestSignatureAlgorithm(
  const TestPublicKeyAlgorithm& publicKeyAlg,
  TestDigestAlgorithmID digestAlg,
  const ByteString& algorithmIdentifier,
  bool accepted)
  : publicKeyAlg(publicKeyAlg)
  , digestAlg(digestAlg)
  , algorithmIdentifier(algorithmIdentifier)
  , accepted(accepted)
{
}


TestPublicKeyAlgorithm
RSA_PKCS1()
{
  static const uint8_t rsaEncryption[] = { PREFIX_1_2_840_113549, 1, 1, 1 };
  return TestPublicKeyAlgorithm(SimpleAlgID(rsaEncryption, NULLParam::YES));
}


TestSignatureAlgorithm md2WithRSAEncryption()
{
  static const uint8_t oidValue[] = { PREFIX_1_2_840_113549, 1, 1, 2 };
  return TestSignatureAlgorithm(RSA_PKCS1(), TestDigestAlgorithmID::MD2,
                                SimpleAlgID(oidValue), false);
}


TestSignatureAlgorithm md5WithRSAEncryption()
{
  static const uint8_t oidValue[] = { PREFIX_1_2_840_113549, 1, 1, 4 };
  return TestSignatureAlgorithm(RSA_PKCS1(), TestDigestAlgorithmID::MD5,
                                SimpleAlgID(oidValue), false);
}


TestSignatureAlgorithm sha1WithRSAEncryption()
{
  static const uint8_t oidValue[] = { PREFIX_1_2_840_113549, 1, 1, 5 };
  return TestSignatureAlgorithm(RSA_PKCS1(), TestDigestAlgorithmID::SHA1,
                                SimpleAlgID(oidValue), true);
}


TestSignatureAlgorithm sha256WithRSAEncryption()
{
  static const uint8_t oidValue[] = { PREFIX_1_2_840_113549, 1, 1, 11 };
  return TestSignatureAlgorithm(RSA_PKCS1(), TestDigestAlgorithmID::SHA256,
                                SimpleAlgID(oidValue), true);
}

} } } 
