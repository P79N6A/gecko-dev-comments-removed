























#ifndef mozilla_pkix_test__pkixtestutils_h
#define mozilla_pkix_test__pkixtestutils_h

#include <ctime>
#include <stdint.h> 
#include <string>

#include "pkix/enumclass.h"
#include "pkix/pkixtypes.h"
#include "pkix/ScopedPtr.h"

namespace mozilla { namespace pkix { namespace test {

typedef std::basic_string<uint8_t> ByteString;

inline bool ENCODING_FAILED(const ByteString& bs) { return bs.empty(); }














#define MOZILLA_PKIX_ARRAY_LENGTH(x) (sizeof(x) / sizeof((x)[0]))

class TestInput : public Input
{
public:
  template <size_t N>
  explicit TestInput(const char (&valueString)[N])
    : Input(reinterpret_cast<const uint8_t(&)[N-1]>(valueString))
  {
  }
};


static const uint8_t tlv_id_kp_OCSPSigning[] = {
  0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x09
};


static const uint8_t tlv_id_kp_serverAuth[] = {
  0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x03, 0x01
};


const uint8_t alg_sha256WithRSAEncryption[] = {
  0x30, 0x0b, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b
};

const ByteString sha256WithRSAEncryption(alg_sha256WithRSAEncryption,
  MOZILLA_PKIX_ARRAY_LENGTH(alg_sha256WithRSAEncryption));


const uint8_t alg_md5WithRSAEncryption[] = {
  0x30, 0x0b, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x04
};

const ByteString md5WithRSAEncryption(alg_md5WithRSAEncryption,
  MOZILLA_PKIX_ARRAY_LENGTH(alg_md5WithRSAEncryption));


const uint8_t alg_md2WithRSAEncryption[] = {
  0x30, 0x0b, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x02
};

const ByteString md2WithRSAEncryption(alg_md2WithRSAEncryption,
  MOZILLA_PKIX_ARRAY_LENGTH(alg_md2WithRSAEncryption));


mozilla::pkix::Time YMDHMS(int16_t year, int16_t month, int16_t day,
                           int16_t hour, int16_t minutes, int16_t seconds);

ByteString CNToDERName(const char* cn);
bool InputEqualsByteString(Input input, const ByteString& bs);

class TestKeyPair
{
public:
  virtual ~TestKeyPair() { }

  
  
  const ByteString subjectPublicKeyInfo;

  
  
  const ByteString subjectPublicKey;

  virtual Result SignData(const ByteString& tbs,
                          const ByteString& signatureAlgorithm,
                           ByteString& signature) const = 0;

  virtual TestKeyPair* Clone() const = 0;
protected:
  TestKeyPair(const ByteString& spki, const ByteString& spk)
    : subjectPublicKeyInfo(spki)
    , subjectPublicKey(spk)
  {
  }

  TestKeyPair(const TestKeyPair&) ;
  void operator=(const TestKeyPair&) ;
};

TestKeyPair* GenerateKeyPair();
inline void DeleteTestKeyPair(TestKeyPair* keyPair) { delete keyPair; }
typedef ScopedPtr<TestKeyPair, DeleteTestKeyPair> ScopedTestKeyPair;

ByteString SHA1(const ByteString& toHash);

Result TestCheckPublicKey(Input subjectPublicKeyInfo);
Result TestVerifySignedData(const SignedDataWithSignature& signedData,
                            Input subjectPublicKeyInfo);
Result TestDigestBuf(Input item,  uint8_t* digestBuf,
                     size_t digestBufLen);









Result TamperOnce( ByteString& item, const ByteString& from,
                  const ByteString& to);




enum Version { v1 = 0, v2 = 1, v3 = 2 };















ByteString CreateEncodedCertificate(long version, const ByteString& signature,
                                    const ByteString& serialNumber,
                                    const ByteString& issuerNameDER,
                                    time_t notBefore, time_t notAfter,
                                    const ByteString& subjectNameDER,
                                     const ByteString* extensions,
                                     TestKeyPair* issuerKeyPair,
                                    const ByteString& signatureAlgorithm,
                                     ScopedTestKeyPair& keyPairResult);

ByteString CreateEncodedSerialNumber(long value);

MOZILLA_PKIX_ENUM_CLASS ExtensionCriticality { NotCritical = 0, Critical = 1 };

ByteString CreateEncodedBasicConstraints(bool isCA,
                                          long* pathLenConstraint,
                                         ExtensionCriticality criticality);


ByteString CreateEncodedEKUExtension(Input eku,
                                     ExtensionCriticality criticality);




class OCSPResponseExtension
{
public:
  ByteString id;
  bool critical;
  ByteString value;
  OCSPResponseExtension* next;
};

class OCSPResponseContext
{
public:
  OCSPResponseContext(const CertID& certID, std::time_t time);

  const CertID& certID;
  

  

  enum OCSPResponseStatus {
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
  ByteString signatureAlgorithm; 
  bool badSignature; 
  const ByteString* certs; 

  
  
  enum CertStatus {
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
