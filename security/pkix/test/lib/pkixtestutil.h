























#ifndef mozilla_pkix_test_pkixtestutils_h
#define mozilla_pkix_test_pkixtestutils_h

#include <ctime>
#include <stdint.h> 
#include <string>

#include "pkix/pkixtypes.h"
#include "../../lib/ScopedPtr.h"

namespace mozilla { namespace pkix { namespace test {

typedef std::basic_string<uint8_t> ByteString;

inline bool ENCODING_FAILED(const ByteString& bs) { return bs.empty(); }














#define MOZILLA_PKIX_ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

bool InputEqualsByteString(Input input, const ByteString& bs);
ByteString InputToByteString(Input input);


static const uint8_t tlv_id_kp_OCSPSigning[] = {
  0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x09
};


static const uint8_t tlv_id_kp_serverAuth[] = {
  0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01
};

enum class TestDigestAlgorithmID
{
  MD2,
  MD5,
  SHA1,
  SHA224,
  SHA256,
  SHA384,
  SHA512,
};

struct TestPublicKeyAlgorithm
{
  explicit TestPublicKeyAlgorithm(const ByteString& algorithmIdentifier)
    : algorithmIdentifier(algorithmIdentifier) { }
  bool operator==(const TestPublicKeyAlgorithm& other) const
  {
    return algorithmIdentifier == other.algorithmIdentifier;
  }
  ByteString algorithmIdentifier;
};

ByteString DSS_P();
ByteString DSS_Q();
ByteString DSS_G();

TestPublicKeyAlgorithm DSS();
TestPublicKeyAlgorithm RSA_PKCS1();

struct TestSignatureAlgorithm
{
  TestSignatureAlgorithm(const TestPublicKeyAlgorithm& publicKeyAlg,
                         TestDigestAlgorithmID digestAlg,
                         const ByteString& algorithmIdentifier,
                         bool accepted);

  TestPublicKeyAlgorithm publicKeyAlg;
  TestDigestAlgorithmID digestAlg;
  ByteString algorithmIdentifier;
  bool accepted;
};

TestSignatureAlgorithm md2WithRSAEncryption();
TestSignatureAlgorithm md5WithRSAEncryption();
TestSignatureAlgorithm sha1WithRSAEncryption();
TestSignatureAlgorithm sha256WithRSAEncryption();


mozilla::pkix::Time YMDHMS(uint16_t year, uint16_t month, uint16_t day,
                           uint16_t hour, uint16_t minutes, uint16_t seconds);

ByteString TLV(uint8_t tag, size_t length, const ByteString& value);

inline ByteString
TLV(uint8_t tag, const ByteString& value)
{
  return TLV(tag, value.length(), value);
}





template <size_t N>
inline ByteString
TLV(uint8_t tag, const char(&value)[N])
{
  static_assert(N > 0, "cannot have string literal of size 0");
  assert(value[N - 1] == 0);
  return TLV(tag, ByteString(reinterpret_cast<const uint8_t*>(&value), N - 1));
}

template <size_t N>
inline ByteString
TLV(uint8_t tag, size_t length, const char(&value)[N])
{
  static_assert(N > 0, "cannot have string literal of size 0");
  assert(value[N - 1] == 0);
  return TLV(tag, length,
             ByteString(reinterpret_cast<const uint8_t*>(&value), N - 1));
}

ByteString Boolean(bool value);
ByteString Integer(long value);

ByteString CN(const ByteString&, uint8_t encodingTag = 0x0c );

inline ByteString
CN(const char* value, uint8_t encodingTag = 0x0c )
{
  return CN(ByteString(reinterpret_cast<const uint8_t*>(value),
                       std::strlen(value)), encodingTag);
}

ByteString OU(const ByteString&);

inline ByteString
OU(const char* value)
{
  return OU(ByteString(reinterpret_cast<const uint8_t*>(value),
                       std::strlen(value)));
}

ByteString emailAddress(const ByteString&);

inline ByteString
emailAddress(const char* value)
{
  return emailAddress(ByteString(reinterpret_cast<const uint8_t*>(value),
                                 std::strlen(value)));
}




ByteString RDN(const ByteString& avas);






ByteString Name(const ByteString& rdns);

inline ByteString
CNToDERName(const ByteString& cn)
{
  return Name(RDN(CN(cn)));
}

inline ByteString
CNToDERName(const char* cn)
{
  return Name(RDN(CN(cn)));
}












inline ByteString
RFC822Name(const ByteString& name)
{
  
  return TLV((2 << 6) | 1, name);
}

template <size_t L>
inline ByteString
RFC822Name(const char (&bytes)[L])
{
  return RFC822Name(ByteString(reinterpret_cast<const uint8_t*>(&bytes),
                               L - 1));
}

inline ByteString
DNSName(const ByteString& name)
{
  
  return TLV((2 << 6) | 2, name);
}

template <size_t L>
inline ByteString
DNSName(const char (&bytes)[L])
{
  return DNSName(ByteString(reinterpret_cast<const uint8_t*>(&bytes),
                            L - 1));
}

inline ByteString
IPAddress()
{
  
  return TLV((2 << 6) | 7, ByteString());
}

template <size_t L>
inline ByteString
IPAddress(const uint8_t (&bytes)[L])
{
  
  return TLV((2 << 6) | 7, ByteString(bytes, L));
}







ByteString CreateEncodedSubjectAltName(const ByteString& names);
ByteString CreateEncodedEmptySubjectAltName();

class TestKeyPair
{
public:
  virtual ~TestKeyPair() { }

  const TestPublicKeyAlgorithm publicKeyAlg;

  
  
  const ByteString subjectPublicKeyInfo;

  
  
  const ByteString subjectPublicKey;

  virtual Result SignData(const ByteString& tbs,
                          const TestSignatureAlgorithm& signatureAlgorithm,
                           ByteString& signature) const = 0;

  virtual TestKeyPair* Clone() const = 0;
protected:
  TestKeyPair(const TestPublicKeyAlgorithm& publicKeyAlg, const ByteString& spk);
  TestKeyPair(const TestKeyPair&) = delete;
  void operator=(const TestKeyPair&) = delete;
};

TestKeyPair* CloneReusedKeyPair();
TestKeyPair* GenerateKeyPair();
TestKeyPair* GenerateDSSKeyPair();
inline void DeleteTestKeyPair(TestKeyPair* keyPair) { delete keyPair; }
typedef ScopedPtr<TestKeyPair, DeleteTestKeyPair> ScopedTestKeyPair;

Result TestVerifyECDSASignedDigest(const SignedDigest& signedDigest,
                                   Input subjectPublicKeyInfo);
Result TestVerifyRSAPKCS1SignedDigest(const SignedDigest& signedDigest,
                                      Input subjectPublicKeyInfo);
Result TestDigestBuf(Input item, DigestAlgorithm digestAlg,
                      uint8_t* digestBuf, size_t digestBufLen);









Result TamperOnce( ByteString& item, const ByteString& from,
                  const ByteString& to);




enum Version { v1 = 0, v2 = 1, v3 = 2 };











ByteString CreateEncodedCertificate(long version,
                                    const TestSignatureAlgorithm& signature,
                                    const ByteString& serialNumber,
                                    const ByteString& issuerNameDER,
                                    time_t notBefore, time_t notAfter,
                                    const ByteString& subjectNameDER,
                                    const TestKeyPair& subjectKeyPair,
                                     const ByteString* extensions,
                                    const TestKeyPair& issuerKeyPair,
                                    const TestSignatureAlgorithm& signatureAlgorithm);

ByteString CreateEncodedSerialNumber(long value);

enum class Critical { No = 0, Yes = 1 };

ByteString CreateEncodedBasicConstraints(bool isCA,
                                          long* pathLenConstraint,
                                         Critical critical);


ByteString CreateEncodedEKUExtension(Input eku, Critical critical);




class OCSPResponseExtension final
{
public:
  ByteString id;
  bool critical;
  ByteString value;
  OCSPResponseExtension* next;
};

class OCSPResponseContext final
{
public:
  OCSPResponseContext(const CertID& certID, std::time_t time);

  const CertID& certID;
  

  

  enum OCSPResponseStatus
  {
    successful = 0,
    malformedRequest = 1,
    internalError = 2,
    tryLater = 3,
    
    sigRequired = 5,
    unauthorized = 6,
  };
  uint8_t responseStatus; 
  bool skipResponseBytes; 

  
  ByteString signerNameDER; 
                            
                            

  std::time_t producedAt;

  OCSPResponseExtension* extensions;
  bool includeEmptyExtensions; 
                               
                               
  ScopedTestKeyPair signerKeyPair;
  TestSignatureAlgorithm signatureAlgorithm;
  bool badSignature; 
  const ByteString* certs; 

  
  
  enum CertStatus
  {
    good = 0,
    revoked = 1,
    unknown = 2,
  };
  uint8_t certStatus; 
  std::time_t revocationTime; 
  std::time_t thisUpdate;
  std::time_t nextUpdate;
  bool includeNextUpdate;
};

ByteString CreateEncodedOCSPResponse(OCSPResponseContext& context);

} } } 

#endif 
