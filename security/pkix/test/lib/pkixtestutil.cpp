























#include "pkixtestutil.h"

#include <cerrno>
#include <cstdio>
#include <limits>
#include <new>

#include "cert.h"
#include "cryptohi.h"
#include "hasht.h"
#include "pk11pub.h"
#include "pkix/pkixnss.h"
#include "pkixder.h"
#include "pkixutil.h"
#include "prinit.h"
#include "prprf.h"
#include "secerr.h"

using namespace std;

namespace mozilla { namespace pkix { namespace test {


static const uint8_t alg_sha256WithRSAEncryption[] = {
  0x30, 0x0b, 0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x01, 0x0b
};
const Input sha256WithRSAEncryption(alg_sha256WithRSAEncryption);

namespace {

inline void
deleteCharArray(char* chars)
{
  delete[] chars;
}

inline void
fclose_void(FILE* file) {
  (void) fclose(file);
}

typedef mozilla::pkix::ScopedPtr<FILE, fclose_void> ScopedFILE;

FILE*
OpenFile(const char* dir, const char* filename, const char* mode)
{
  assert(dir);
  assert(*dir);
  assert(filename);
  assert(*filename);

  ScopedPtr<char, deleteCharArray>
    path(new (nothrow) char[strlen(dir) + 1 + strlen(filename) + 1]);
  if (!path) {
    return nullptr;
  }
  strcpy(path.get(), dir);
  strcat(path.get(), "/");
  strcat(path.get(), filename);

  ScopedFILE file;
#ifdef _MSC_VER
  {
    FILE* rawFile;
    errno_t error = fopen_s(&rawFile, path.get(), mode);
    if (error) {
      
      rawFile = nullptr;
    }
    file = rawFile;
  }
#else
  file = fopen(path.get(), mode);
#endif
  return file.release();
}

} 

Result
TamperOnce( ByteString& item, const ByteString& from,
           const ByteString& to)
{
  if (from.length() < 8) {
    return Result::FATAL_ERROR_INVALID_ARGS;
  }
  if (from.length() != to.length()) {
    return Result::FATAL_ERROR_INVALID_ARGS;
  }
  size_t pos = item.find(from);
  if (pos == string::npos) {
    return Result::FATAL_ERROR_INVALID_ARGS; 
  }
  if (item.find(from, pos + from.length()) != string::npos) {
    return Result::FATAL_ERROR_INVALID_ARGS; 
  }
  item.replace(pos, from.length(), to);
  return Success;
}


const ByteString ENCODING_FAILED;


static ByteString
TLV(uint8_t tag, const ByteString& value)
{
  ByteString result;
  result.push_back(tag);

  if (value.length() < 128) {
    result.push_back(value.length());
  } else if (value.length() < 256) {
    result.push_back(0x81u);
    result.push_back(value.length());
  } else if (value.length() < 65536) {
    result.push_back(0x82u);
    result.push_back(static_cast<uint8_t>(value.length() / 256));
    result.push_back(static_cast<uint8_t>(value.length() % 256));
  } else {
    assert(false);
    return ENCODING_FAILED;
  }
  result.append(value);
  return result;
}

static SECItem*
ArenaDupByteString(PLArenaPool* arena, const ByteString& value)
{
  SECItem* result = SECITEM_AllocItem(arena, nullptr, value.length());
  if (!result) {
    return nullptr;
  }
  memcpy(result->data, value.data(), value.length());
  return result;
}

class Output
{
public:
  Output()
  {
  }

  
  
  Result Add(const SECItem* item)
  {
    assert(item);
    assert(item->data);
    contents.append(item->data, item->len);
    return Success; 
  }

  void Add(const ByteString& item)
  {
    contents.append(item);
  }

  SECItem* Squash(PLArenaPool* arena, uint8_t tag)
  {
    assert(arena);

    size_t lengthLength = contents.length() < 128 ? 1
                        : contents.length() < 256 ? 2
                        : 3;
    size_t totalLength = 1 + lengthLength + contents.length();
    SECItem* output = SECITEM_AllocItem(arena, nullptr, totalLength);
    if (!output) {
      return nullptr;
    }
    uint8_t* d = output->data;
    *d++ = tag;
    EncodeLength(d, contents.length(), lengthLength);
    d += lengthLength;
    memcpy(d, contents.data(), contents.length());
    return output;
  }

private:
  void
  EncodeLength(uint8_t* data, size_t length, size_t lengthLength)
  {
    switch (lengthLength) {
      case 1:
        data[0] = length;
        break;
      case 2:
        data[0] = 0x81;
        data[1] = length;
        break;
      case 3:
        data[0] = 0x82;
        data[1] = length / 256;
        data[2] = length % 256;
        break;
      default:
        abort();
    }
  }

  ByteString contents;
};

OCSPResponseContext::OCSPResponseContext(PLArenaPool* arena,
                                         const CertID& certID, time_t time)
  : arena(arena)
  , certID(certID)
  , responseStatus(successful)
  , skipResponseBytes(false)
  , producedAt(time)
  , extensions(nullptr)
  , includeEmptyExtensions(false)
  , badSignature(false)
  , certs(nullptr)

  , certStatus(good)
  , revocationTime(0)
  , thisUpdate(time)
  , nextUpdate(time + 10)
  , includeNextUpdate(true)
{
}

static ByteString ResponseBytes(OCSPResponseContext& context);
static ByteString BasicOCSPResponse(OCSPResponseContext& context);
static SECItem* ResponseData(OCSPResponseContext& context);
static ByteString ResponderID(OCSPResponseContext& context);
static ByteString KeyHash(OCSPResponseContext& context);
static ByteString SingleResponse(OCSPResponseContext& context);
static ByteString CertID(OCSPResponseContext& context);
static ByteString CertStatus(OCSPResponseContext& context);

static ByteString
HashedOctetString(const SECItem& bytes)
{
  uint8_t hashBuf[TrustDomain::DIGEST_LENGTH];
  Input input;
  if (input.Init(bytes.data, bytes.len) != Success) {
    return ENCODING_FAILED;
  }
  if (DigestBuf(input, hashBuf, sizeof(hashBuf)) != Success) {
    return ENCODING_FAILED;
  }
  return TLV(der::OCTET_STRING, ByteString(hashBuf, sizeof(hashBuf)));
}

static ByteString
KeyHashHelper(const CERTSubjectPublicKeyInfo* spki)
{
  
  SECItem spk = spki->subjectPublicKey;
  DER_ConvertBitString(&spk); 
  return HashedOctetString(spk);
}

static ByteString
BitString(const ByteString& rawBytes, bool corrupt)
{
  ByteString prefixed;
  
  
  
  prefixed.push_back(0);
  prefixed.append(rawBytes);
  if (corrupt) {
    assert(prefixed.length() > 8);
    prefixed[8]++;
  }
  return TLV(der::BIT_STRING, prefixed);
}

static ByteString
Boolean(bool value)
{
  ByteString encodedValue;
  encodedValue.push_back(value ? 0xff : 0x00);
  return TLV(der::BOOLEAN, encodedValue);
}

static ByteString
Integer(long value)
{
  if (value < 0 || value > 127) {
    
    return ENCODING_FAILED;
  }

  ByteString encodedValue;
  encodedValue.push_back(static_cast<uint8_t>(value));
  return TLV(der::INTEGER, encodedValue);
}

enum TimeEncoding { UTCTime = 0, GeneralizedTime = 1 };


#ifdef WIN32
static tm*
gmtime_r(const time_t* t,  tm* exploded)
{
  if (gmtime_s(exploded, t) != 0) {
    return nullptr;
  }
  return exploded;
}
#endif







static ByteString
TimeToEncodedTime(time_t time, TimeEncoding encoding)
{
  assert(encoding == UTCTime || encoding == GeneralizedTime);

  tm exploded;
  if (!gmtime_r(&time, &exploded)) {
    return ENCODING_FAILED;
  }

  if (exploded.tm_sec >= 60) {
    
    exploded.tm_sec = 59;
  }

  
  int year = exploded.tm_year + 1900;

  if (encoding == UTCTime && (year < 1950 || year >= 2050)) {
    return ENCODING_FAILED;
  }

  ByteString value;

  if (encoding == GeneralizedTime) {
    value.push_back('0' + (year / 1000));
    value.push_back('0' + ((year % 1000) / 100));
  }

  value.push_back('0' + ((year % 100) / 10));
  value.push_back('0' + (year % 10));
  value.push_back('0' + ((exploded.tm_mon + 1) / 10));
  value.push_back('0' + ((exploded.tm_mon + 1) % 10));
  value.push_back('0' + (exploded.tm_mday / 10));
  value.push_back('0' + (exploded.tm_mday % 10));
  value.push_back('0' + (exploded.tm_hour / 10));
  value.push_back('0' + (exploded.tm_hour % 10));
  value.push_back('0' + (exploded.tm_min / 10));
  value.push_back('0' + (exploded.tm_min % 10));
  value.push_back('0' + (exploded.tm_sec / 10));
  value.push_back('0' + (exploded.tm_sec % 10));
  value.push_back('Z');

  return TLV(encoding == GeneralizedTime ? der::GENERALIZED_TIME : der::UTCTime,
             value);
}

static ByteString
TimeToGeneralizedTime(time_t time)
{
  return TimeToEncodedTime(time, GeneralizedTime);
}






static ByteString
TimeToTimeChoice(time_t time)
{
  tm exploded;
  if (!gmtime_r(&time, &exploded)) {
    return ENCODING_FAILED;
  }
  TimeEncoding encoding = (exploded.tm_year + 1900 >= 1950 &&
                           exploded.tm_year + 1900 < 2050)
                        ? UTCTime
                        : GeneralizedTime;

  return TimeToEncodedTime(time, encoding);
}

Time
YMDHMS(int16_t year, int16_t month, int16_t day,
       int16_t hour, int16_t minutes, int16_t seconds)
{
  assert(year <= 9999);
  assert(month >= 1);
  assert(month <= 12);
  assert(day >= 1);
  assert(hour >= 0);
  assert(hour < 24);
  assert(minutes >= 0);
  assert(minutes < 60);
  assert(seconds >= 0);
  assert(seconds < 60);

  uint64_t days = DaysBeforeYear(year);

  {
    static const int16_t DAYS_IN_MONTH[] = {
      31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31
    };

    int16_t i = 1;
    for (;;) {
      int16_t daysInMonth = DAYS_IN_MONTH[i - 1];
      if (i == 2 &&
          ((year % 4 == 0) && ((year % 100 != 0) || (year % 400 == 0)))) {
        
        ++daysInMonth;
      }
      if (i == month) {
        assert(day <= daysInMonth);
        break;
      }
      days += daysInMonth;
      ++i;
    }
  }

  days += (day - 1);

  uint64_t totalSeconds = days * Time::ONE_DAY_IN_SECONDS;
  totalSeconds += hour * 60 * 60;
  totalSeconds += minutes * 60;
  totalSeconds += seconds;
  return TimeFromElapsedSecondsAD(totalSeconds);
}

static ByteString
SignedData(const ByteString& tbsData,
           SECKEYPrivateKey* privKey,
           SignatureAlgorithm signatureAlgorithm,
           bool corrupt,  SECItem const* const* certs)
{
  assert(privKey);
  if (!privKey) {
    return ENCODING_FAILED;
  }

  SECOidTag signatureAlgorithmOidTag;
  ByteString signatureAlgorithmDER;
  switch (signatureAlgorithm) {
    case SignatureAlgorithm::rsa_pkcs1_with_sha256:
      signatureAlgorithmOidTag = SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION;
      signatureAlgorithmDER.assign(alg_sha256WithRSAEncryption,
                                   sizeof(alg_sha256WithRSAEncryption));
      break;
    default:
      return ENCODING_FAILED;
  }

  SECItem signature;
  if (SEC_SignData(&signature, tbsData.data(), tbsData.length(), privKey,
                   signatureAlgorithmOidTag) != SECSuccess)
  {
    return nullptr;
  }
  
  
  ByteString signatureNested(BitString(ByteString(signature.data, signature.len),
                                       corrupt));
  SECITEM_FreeItem(&signature, false);
  if (signatureNested == ENCODING_FAILED) {
    return nullptr;
  }

  ByteString certsNested;
  if (certs) {
    ByteString certsSequenceValue;
    while (*certs) {
      certsSequenceValue.append(ByteString((*certs)->data, (*certs)->len));
      ++certs;
    }
    ByteString certsSequence(TLV(der::SEQUENCE, certsSequenceValue));
    if (certsSequence == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    certsNested = TLV(der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0,
                      certsSequence);
    if (certsNested == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
  }

  ByteString value;
  value.append(tbsData);
  value.append(signatureAlgorithmDER);
  value.append(signatureNested);
  value.append(certsNested);
  return TLV(der::SEQUENCE, value);
}









static ByteString
Extension(Input extnID, ExtensionCriticality criticality,
          const ByteString& extnValueBytes)
{
  ByteString encoded;

  encoded.append(ByteString(extnID.UnsafeGetData(), extnID.GetLength()));

  if (criticality == ExtensionCriticality::Critical) {
    ByteString critical(Boolean(true));
    if (critical == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    encoded.append(critical);
  }

  ByteString extnValueSequence(TLV(der::SEQUENCE, extnValueBytes));
  if (extnValueBytes == ENCODING_FAILED) {
    return ENCODING_FAILED;
  }
  ByteString extnValue(TLV(der::OCTET_STRING, extnValueSequence));
  if (extnValue == ENCODING_FAILED) {
    return ENCODING_FAILED;
  }
  encoded.append(extnValue);
  return TLV(der::SEQUENCE, encoded);
}

void
MaybeLogOutput(const ByteString& result, const char* suffix)
{
  assert(suffix);

  
  
  
  const char* logPath = getenv("MOZILLA_PKIX_TEST_LOG_DIR");
  if (logPath) {
    static int counter = 0;
    ScopedPtr<char, PR_smprintf_free>
      filename(PR_smprintf("%u-%s.der", counter, suffix));
    ++counter;
    if (filename) {
      ScopedFILE file(OpenFile(logPath, filename.get(), "wb"));
      if (file) {
        (void) fwrite(result.data(), result.length(), 1, file.get());
      }
    }
  }
}




Result
GenerateKeyPair( ScopedSECKEYPublicKey& publicKey,
                 ScopedSECKEYPrivateKey& privateKey)
{
  ScopedPtr<PK11SlotInfo, PK11_FreeSlot> slot(PK11_GetInternalSlot());
  if (!slot) {
    return MapPRErrorCodeToResult(PR_GetError());
  }

  
  
  
  for (uint32_t retries = 0; retries < 10; retries++) {
    PK11RSAGenParams params;
    params.keySizeInBits = 2048;
    params.pe = 3;
    SECKEYPublicKey* publicKeyTemp = nullptr;
    privateKey = PK11_GenerateKeyPair(slot.get(), CKM_RSA_PKCS_KEY_PAIR_GEN,
                                      &params, &publicKeyTemp, false, true,
                                      nullptr);
    if (privateKey) {
      publicKey = publicKeyTemp;
      assert(publicKey);
      return Success;
    }

    assert(!publicKeyTemp);

    if (PR_GetError() != SEC_ERROR_PKCS11_FUNCTION_FAILED) {
      break;
    }

    
    
    
    static const uint8_t RANDOM_NUMBER[] = { 4, 4, 4, 4, 4, 4, 4, 4 };
    if (PK11_RandomUpdate((void*) &RANDOM_NUMBER,
                          sizeof(RANDOM_NUMBER)) != SECSuccess) {
      break;
    }
  }

  return MapPRErrorCodeToResult(PR_GetError());
}





static ByteString TBSCertificate(long version, const ByteString& serialNumber,
                                 Input signature, const ByteString& issuer,
                                 time_t notBefore, time_t notAfter,
                                 const ByteString& subject,
                                 const SECKEYPublicKey* subjectPublicKey,
                                  const ByteString* extensions);





SECItem*
CreateEncodedCertificate(PLArenaPool* arena, long version, Input signature,
                         const ByteString& serialNumber,
                         const ByteString& issuerNameDER,
                         time_t notBefore, time_t notAfter,
                         const ByteString& subjectNameDER,
                          const ByteString* extensions,
                          SECKEYPrivateKey* issuerPrivateKey,
                         SignatureAlgorithm signatureAlgorithm,
                          ScopedSECKEYPrivateKey& privateKeyResult)
{
  assert(arena);
  if (!arena) {
    return nullptr;
  }

  
  
  
  ScopedSECKEYPublicKey publicKey;
  ScopedSECKEYPrivateKey privateKeyTemp;
  if (GenerateKeyPair(publicKey, privateKeyTemp) != Success) {
    return nullptr;
  }

  ByteString tbsCertificate(TBSCertificate(version, serialNumber,
                                           signature, issuerNameDER, notBefore,
                                           notAfter, subjectNameDER,
                                           publicKey.get(), extensions));
  if (tbsCertificate == ENCODING_FAILED) {
    return nullptr;
  }

  ByteString result(SignedData(tbsCertificate,
                               issuerPrivateKey ? issuerPrivateKey
                                                : privateKeyTemp.get(),
                               signatureAlgorithm, false, nullptr));
  if (result == ENCODING_FAILED) {
    return nullptr;
  }

  MaybeLogOutput(result, "cert");

  privateKeyResult = privateKeyTemp.release();

  return ArenaDupByteString(arena, result);
}















static ByteString
TBSCertificate(long versionValue,
               const ByteString& serialNumber, Input signature,
               const ByteString& issuer, time_t notBeforeTime,
               time_t notAfterTime, const ByteString& subject,
               const SECKEYPublicKey* subjectPublicKey,
                const ByteString* extensions)
{
  assert(subjectPublicKey);
  if (!subjectPublicKey) {
    return ENCODING_FAILED;
  }

  ByteString value;

  if (versionValue != static_cast<long>(der::Version::v1)) {
    ByteString versionInteger(Integer(versionValue));
    if (versionInteger == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    ByteString version(TLV(der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0,
                           versionInteger));
    if (version == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    value.append(version);
  }

  value.append(serialNumber);
  value.append(signature.UnsafeGetData(), signature.GetLength());
  value.append(issuer);

  
  
  
  ByteString validity;
  {
    ByteString notBefore(TimeToTimeChoice(notBeforeTime));
    if (notBefore == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    ByteString notAfter(TimeToTimeChoice(notAfterTime));
    if (notAfter == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    ByteString validityValue;
    validityValue.append(notBefore);
    validityValue.append(notAfter);
    validity = TLV(der::SEQUENCE, validityValue);
    if (validity == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
  }
  value.append(validity);

  value.append(subject);

  
  
  
  ScopedSECItem subjectPublicKeyInfo(
    SECKEY_EncodeDERSubjectPublicKeyInfo(subjectPublicKey));
  if (!subjectPublicKeyInfo) {
    return ENCODING_FAILED;
  }
  value.append(subjectPublicKeyInfo->data, subjectPublicKeyInfo->len);

  if (extensions) {
    ByteString extensionsValue;
    while (!(*extensions).empty()) {
      extensionsValue.append(*extensions);
      ++extensions;
    }
    ByteString extensionsSequence(TLV(der::SEQUENCE, extensionsValue));
    if (extensionsSequence == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    ByteString extensionsWrapped(
      TLV(der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 3, extensionsSequence));
    if (extensionsWrapped == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    value.append(extensionsWrapped);
  }

  return TLV(der::SEQUENCE, value);
}

ByteString
CNToDERName(const char* cn)
{
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  
  

  
  static const uint8_t tlv_id_at_commonName[] = {
    0x06, 0x03, 0x55, 0x04, 0x03
  };

  ByteString value(reinterpret_cast<const ByteString::value_type*>(cn));
  value = TLV(der::UTF8String, value);

  ByteString ava;
  ava.append(tlv_id_at_commonName, sizeof(tlv_id_at_commonName));
  ava.append(value);
  ava = TLV(der::SEQUENCE, ava);
  if (ava == ENCODING_FAILED) {
    return ENCODING_FAILED;
  }

  ByteString rdn(TLV(der::SET, ava));
  if (rdn == ENCODING_FAILED) {
    return ENCODING_FAILED;
  }

  return TLV(der::SEQUENCE, rdn);
}

ByteString
CreateEncodedSerialNumber(long serialNumberValue)
{
  return Integer(serialNumberValue);
}




ByteString
CreateEncodedBasicConstraints(bool isCA,
                               long* pathLenConstraintValue,
                              ExtensionCriticality criticality)
{
  ByteString value;

  if (isCA) {
    ByteString cA(Boolean(true));
    if (cA == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    value.append(cA);
  }

  if (pathLenConstraintValue) {
    ByteString pathLenConstraint(Integer(*pathLenConstraintValue));
    if (pathLenConstraint == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    value.append(pathLenConstraint);
  }

  
  static const uint8_t tlv_id_ce_basicConstraints[] = {
    0x06, 0x03, 0x55, 0x1d, 0x13
  };
  return Extension(Input(tlv_id_ce_basicConstraints), criticality, value);
}



ByteString
CreateEncodedEKUExtension(Input ekuOID, ExtensionCriticality criticality)
{
  ByteString value(ekuOID.UnsafeGetData(), ekuOID.GetLength());

  
  static const uint8_t tlv_id_ce_extKeyUsage[] = {
    0x06, 0x03, 0x55, 0x1d, 0x25
  };

  return Extension(Input(tlv_id_ce_extKeyUsage), criticality, value);
}




SECItem*
CreateEncodedOCSPResponse(OCSPResponseContext& context)
{
  if (!context.arena) {
    return nullptr;
  }

  if (!context.skipResponseBytes) {
    if (!context.signerPrivateKey) {
      return nullptr;
    }
  }

  
  
  

  
  
  
  
  
  
  
  
  
  ByteString reponseStatusValue;
  reponseStatusValue.push_back(context.responseStatus);
  ByteString responseStatus(TLV(der::ENUMERATED, reponseStatusValue));
  if (responseStatus == ENCODING_FAILED) {
    return nullptr;
  }

  ByteString responseBytesNested;
  if (!context.skipResponseBytes) {
    ByteString responseBytes(ResponseBytes(context));
    if (responseBytes == ENCODING_FAILED) {
      return nullptr;
    }

    responseBytesNested = TLV(der::CONSTRUCTED | der::CONTEXT_SPECIFIC,
                              responseBytes);
    if (responseBytesNested == ENCODING_FAILED) {
      return nullptr;
    }
  }

  ByteString value;
  value.append(responseStatus);
  value.append(responseBytesNested);
  ByteString result(TLV(der::SEQUENCE, value));
  if (result == ENCODING_FAILED) {
    return nullptr;
  }

  MaybeLogOutput(result, "ocsp");

  return ArenaDupByteString(context.arena, result);
}




ByteString
ResponseBytes(OCSPResponseContext& context)
{
  
  static const uint8_t id_pkix_ocsp_basic_encoded[] = {
    0x06, 0x09, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x01
  };
  ByteString response(BasicOCSPResponse(context));
  if (response == ENCODING_FAILED) {
    return nullptr;
  }
  ByteString responseNested = TLV(der::OCTET_STRING, response);
  if (responseNested == ENCODING_FAILED) {
    return nullptr;
  }

  ByteString value;
  value.append(id_pkix_ocsp_basic_encoded,
               sizeof(id_pkix_ocsp_basic_encoded));
  value.append(responseNested);
  return TLV(der::SEQUENCE, value);
}






ByteString
BasicOCSPResponse(OCSPResponseContext& context)
{
  SECItem* tbsResponseData = ResponseData(context);
  if (!tbsResponseData) {
    return nullptr;
  }

  
  return SignedData(ByteString(tbsResponseData->data, tbsResponseData->len),
                    context.signerPrivateKey.get(),
                    SignatureAlgorithm::rsa_pkcs1_with_sha256,
                    context.badSignature, context.certs);
}






static ByteString
OCSPExtension(OCSPResponseContext& context, OCSPResponseExtension& extension)
{
  ByteString encoded;
  encoded.append(extension.id);
  if (extension.critical) {
    ByteString critical(Boolean(true));
    if (critical == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    encoded.append(critical);
  }
  ByteString value(TLV(der::OCTET_STRING, extension.value));
  if (value == ENCODING_FAILED) {
    return ENCODING_FAILED;
  }
  encoded.append(value);
  return TLV(der::SEQUENCE, encoded);
}




static ByteString
Extensions(OCSPResponseContext& context)
{
  ByteString value;
  for (OCSPResponseExtension* extension = context.extensions;
       extension; extension = extension->next) {
    ByteString extensionEncoded(OCSPExtension(context, *extension));
    if (extensionEncoded == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    value.append(extensionEncoded);
  }
  ByteString sequence(TLV(der::SEQUENCE, value));
  if (sequence == ENCODING_FAILED) {
    return ENCODING_FAILED;
  }
  return TLV(der::CONSTRUCTED | der::CONTEXT_SPECIFIC | 1, sequence);
}







SECItem*
ResponseData(OCSPResponseContext& context)
{
  ByteString responderID(ResponderID(context));
  if (responderID == ENCODING_FAILED) {
    return nullptr;
  }
  ByteString producedAtEncoded(TimeToGeneralizedTime(context.producedAt));
  if (producedAtEncoded == ENCODING_FAILED) {
    return nullptr;
  }
  ByteString response(SingleResponse(context));
  if (response == ENCODING_FAILED) {
    return nullptr;
  }
  ByteString responses(TLV(der::SEQUENCE, response));
  if (responses == ENCODING_FAILED) {
    return nullptr;
  }
  ByteString responseExtensions;
  if (context.extensions || context.includeEmptyExtensions) {
    responseExtensions = Extensions(context);
  }

  Output output;
  output.Add(responderID);
  output.Add(producedAtEncoded);
  output.Add(responses);
  output.Add(responseExtensions);
  return output.Squash(context.arena, der::SEQUENCE);
}





ByteString
ResponderID(OCSPResponseContext& context)
{
  ByteString contents;
  uint8_t responderIDType;
  if (!context.signerNameDER.empty()) {
    contents = context.signerNameDER;
    responderIDType = 1; 
  } else {
    contents = KeyHash(context);
    if (contents == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    responderIDType = 2; 
  }

  return TLV(der::CONSTRUCTED | der::CONTEXT_SPECIFIC | responderIDType,
             contents);
}






ByteString
KeyHash(OCSPResponseContext& context)
{
  ScopedSECKEYPublicKey
    signerPublicKey(SECKEY_ConvertToPublicKey(context.signerPrivateKey.get()));
  if (!signerPublicKey) {
    return nullptr;
  }
  ScopedPtr<CERTSubjectPublicKeyInfo, SECKEY_DestroySubjectPublicKeyInfo>
    signerSPKI(SECKEY_CreateSubjectPublicKeyInfo(signerPublicKey.get()));
  if (!signerSPKI) {
    return nullptr;
  }
  return KeyHashHelper(signerSPKI.get());
}







ByteString
SingleResponse(OCSPResponseContext& context)
{
  ByteString certID(CertID(context));
  if (certID == ENCODING_FAILED) {
    return ENCODING_FAILED;
  }
  ByteString certStatus(CertStatus(context));
  if (certStatus == ENCODING_FAILED) {
    return ENCODING_FAILED;
  }
  ByteString thisUpdateEncoded(TimeToGeneralizedTime(context.thisUpdate));
  if (thisUpdateEncoded == ENCODING_FAILED) {
    return ENCODING_FAILED;
  }
  ByteString nextUpdateEncodedNested;
  if (context.includeNextUpdate) {
    ByteString nextUpdateEncoded(TimeToGeneralizedTime(context.nextUpdate));
    if (nextUpdateEncoded == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
    nextUpdateEncodedNested = TLV(der::CONSTRUCTED | der::CONTEXT_SPECIFIC | 0,
                                  nextUpdateEncoded);
    if (nextUpdateEncodedNested == ENCODING_FAILED) {
      return ENCODING_FAILED;
    }
  }

  ByteString value;
  value.append(certID);
  value.append(certStatus);
  value.append(thisUpdateEncoded);
  value.append(nextUpdateEncodedNested);
  return TLV(der::SEQUENCE, value);
}






ByteString
CertID(OCSPResponseContext& context)
{
  SECItem issuerSECItem = UnsafeMapInputToSECItem(context.certID.issuer);
  ByteString issuerNameHash(HashedOctetString(issuerSECItem));
  if (issuerNameHash == ENCODING_FAILED) {
    return ENCODING_FAILED;
  }

  SECItem issuerSubjectPublicKeyInfoSECItem =
    UnsafeMapInputToSECItem(context.certID.issuerSubjectPublicKeyInfo);
  ScopedPtr<CERTSubjectPublicKeyInfo, SECKEY_DestroySubjectPublicKeyInfo>
    spki(SECKEY_DecodeDERSubjectPublicKeyInfo(
           &issuerSubjectPublicKeyInfoSECItem));
  if (!spki) {
    return ENCODING_FAILED;
  }
  ByteString issuerKeyHash(KeyHashHelper(spki.get()));
  if (issuerKeyHash == ENCODING_FAILED) {
    return ENCODING_FAILED;
  }

  ByteString serialNumberValue(context.certID.serialNumber.UnsafeGetData(),
                               context.certID.serialNumber.GetLength());
  ByteString serialNumber(TLV(der::INTEGER, serialNumberValue));
  if (serialNumber == ENCODING_FAILED) {
    return ENCODING_FAILED;
  }

  
  static const uint8_t alg_id_sha1[] = {
    0x30, 0x07, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1a
  };

  ByteString value;
  value.append(alg_id_sha1, sizeof(alg_id_sha1));
  value.append(issuerNameHash);
  value.append(issuerKeyHash);
  value.append(serialNumber);
  return TLV(der::SEQUENCE, value);
}












ByteString
CertStatus(OCSPResponseContext& context)
{
  switch (context.certStatus) {
    
    
    case 0:
    case 2:
    {
      return TLV(der::CONTEXT_SPECIFIC | context.certStatus, ByteString());
    }
    case 1:
    {
      ByteString revocationTime(TimeToGeneralizedTime(context.revocationTime));
      if (revocationTime == ENCODING_FAILED) {
        return ENCODING_FAILED;
      }
      
      return TLV(der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1, revocationTime);
    }
    default:
      assert(false);
      
  }
  return ENCODING_FAILED;
}

} } } 
