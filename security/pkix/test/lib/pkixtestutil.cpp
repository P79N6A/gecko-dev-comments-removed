























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

Result
InitInputFromSECItem(const SECItem* secItem,  Input& input)
{
  if (!secItem) {
    return Result::FATAL_ERROR_INVALID_ARGS;
  }
  return input.Init(secItem->data, secItem->len);
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

static SECItem* ResponseBytes(OCSPResponseContext& context);
static SECItem* BasicOCSPResponse(OCSPResponseContext& context);
static SECItem* ResponseData(OCSPResponseContext& context);
static ByteString ResponderID(OCSPResponseContext& context);
static ByteString KeyHash(OCSPResponseContext& context);
static SECItem* SingleResponse(OCSPResponseContext& context);
static SECItem* CertID(OCSPResponseContext& context);
static SECItem* CertStatus(OCSPResponseContext& context);

static SECItem*
EncodeNested(PLArenaPool* arena, uint8_t tag, const SECItem* inner)
{
  Output output;
  if (output.Add(inner) != Success) {
    return nullptr;
  }
  return output.Squash(arena, tag);
}

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

static SECItem*
Boolean(PLArenaPool* arena, bool value)
{
  assert(arena);
  SECItem* result(SECITEM_AllocItem(arena, nullptr, 3));
  if (!result) {
    return nullptr;
  }
  result->data[0] = der::BOOLEAN;
  result->data[1] = 1; 
  result->data[2] = value ? 0xff : 0x00;
  return result;
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







static SECItem*
TimeToEncodedTime(PLArenaPool* arena, time_t time, TimeEncoding encoding)
{
  assert(encoding == UTCTime || encoding == GeneralizedTime);

  tm exploded;
  if (!gmtime_r(&time, &exploded)) {
    return nullptr;
  }

  if (exploded.tm_sec >= 60) {
    
    exploded.tm_sec = 59;
  }

  
  int year = exploded.tm_year + 1900;

  if (encoding == UTCTime && (year < 1950 || year >= 2050)) {
    return nullptr;
  }

  SECItem* derTime = SECITEM_AllocItem(arena, nullptr,
                                       encoding == UTCTime ? 15 : 17);
  if (!derTime) {
    return nullptr;
  }

  size_t i = 0;

  derTime->data[i++] = encoding == GeneralizedTime ? 0x18 : 0x17; 
  derTime->data[i++] = static_cast<uint8_t>(derTime->len - 2); 

  if (encoding == GeneralizedTime) {
    derTime->data[i++] = '0' + (year / 1000);
    derTime->data[i++] = '0' + ((year % 1000) / 100);
  }

  derTime->data[i++] = '0' + ((year % 100) / 10);
  derTime->data[i++] = '0' + (year % 10);
  derTime->data[i++] = '0' + ((exploded.tm_mon + 1) / 10);
  derTime->data[i++] = '0' + ((exploded.tm_mon + 1) % 10);
  derTime->data[i++] = '0' + (exploded.tm_mday / 10);
  derTime->data[i++] = '0' + (exploded.tm_mday % 10);
  derTime->data[i++] = '0' + (exploded.tm_hour / 10);
  derTime->data[i++] = '0' + (exploded.tm_hour % 10);
  derTime->data[i++] = '0' + (exploded.tm_min / 10);
  derTime->data[i++] = '0' + (exploded.tm_min % 10);
  derTime->data[i++] = '0' + (exploded.tm_sec / 10);
  derTime->data[i++] = '0' + (exploded.tm_sec % 10);
  derTime->data[i++] = 'Z';

  return derTime;
}

static SECItem*
TimeToGeneralizedTime(PLArenaPool* arena, time_t time)
{
  return TimeToEncodedTime(arena, time, GeneralizedTime);
}






static SECItem*
TimeToTimeChoice(PLArenaPool* arena, time_t time)
{
  tm exploded;
  if (!gmtime_r(&time, &exploded)) {
    return nullptr;
  }
  TimeEncoding encoding = (exploded.tm_year + 1900 >= 1950 &&
                           exploded.tm_year + 1900 < 2050)
                        ? UTCTime
                        : GeneralizedTime;

  return TimeToEncodedTime(arena, time, encoding);
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

static SECItem*
SignedData(PLArenaPool* arena, const SECItem* tbsData,
           SECKEYPrivateKey* privKey,
           SignatureAlgorithm signatureAlgorithm,
           bool corrupt,  SECItem const* const* certs)
{
  assert(arena);
  assert(tbsData);
  assert(privKey);
  if (!arena || !tbsData || !privKey) {
    return nullptr;
  }

  SECOidTag signatureAlgorithmOidTag;
  Input signatureAlgorithmDER;
  switch (signatureAlgorithm) {
    case SignatureAlgorithm::rsa_pkcs1_with_sha256:
      signatureAlgorithmOidTag = SEC_OID_PKCS1_SHA256_WITH_RSA_ENCRYPTION;
      if (signatureAlgorithmDER.Init(alg_sha256WithRSAEncryption,
                                     sizeof(alg_sha256WithRSAEncryption))
            != Success) {
        return nullptr;
      }
      break;
    default:
      return nullptr;
  }

  
  
  SECItem signature;
  if (SEC_SignData(&signature, tbsData->data, tbsData->len, privKey,
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

  SECItem* certsNested = nullptr;
  if (certs) {
    Output certsOutput;
    while (*certs) {
      certsOutput.Add(*certs);
      ++certs;
    }
    SECItem* certsSequence = certsOutput.Squash(arena, der::SEQUENCE);
    if (!certsSequence) {
      return nullptr;
    }
    certsNested = EncodeNested(arena,
                               der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0,
                               certsSequence);
    if (!certsNested) {
      return nullptr;
    }
  }

  Output output;
  if (output.Add(tbsData) != Success) {
    return nullptr;
  }

  SECItem sigantureAlgorithmDERItem =
    UnsafeMapInputToSECItem(signatureAlgorithmDER);
  if (output.Add(&sigantureAlgorithmDERItem) != Success) {
    return nullptr;
  }
  output.Add(signatureNested);
  if (certsNested) {
    if (output.Add(certsNested) != Success) {
      return nullptr;
    }
  }
  return output.Squash(arena, der::SEQUENCE);
}









static SECItem*
Extension(PLArenaPool* arena, Input extnID,
          ExtensionCriticality criticality, Output& value)
{
  assert(arena);
  if (!arena) {
    return nullptr;
  }

  Output output;

  const SECItem extnIDItem = UnsafeMapInputToSECItem(extnID);
  if (output.Add(&extnIDItem) != Success) {
    return nullptr;
  }

  if (criticality == ExtensionCriticality::Critical) {
    SECItem* critical(Boolean(arena, true));
    if (output.Add(critical) != Success) {
      return nullptr;
    }
  }

  SECItem* extnValueBytes(value.Squash(arena, der::SEQUENCE));
  if (!extnValueBytes) {
    return nullptr;
  }
  SECItem* extnValue(EncodeNested(arena, der::OCTET_STRING, extnValueBytes));
  if (!extnValue) {
    return nullptr;
  }
  if (output.Add(extnValue) != Success) {
    return nullptr;
  }

  return output.Squash(arena, der::SEQUENCE);
}

SECItem*
MaybeLogOutput(SECItem* result, const char* suffix)
{
  assert(suffix);

  if (!result) {
    return nullptr;
  }

  
  
  
  const char* logPath = getenv("MOZILLA_PKIX_TEST_LOG_DIR");
  if (logPath) {
    static int counter = 0;
    ScopedPtr<char, PR_smprintf_free>
      filename(PR_smprintf("%u-%s.der", counter, suffix));
    ++counter;
    if (filename) {
      ScopedFILE file(OpenFile(logPath, filename.get(), "wb"));
      if (file) {
        (void) fwrite(result->data, result->len, 1, file.get());
      }
    }
  }

  return result;
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





static SECItem* TBSCertificate(PLArenaPool* arena, long version,
                               const ByteString& serialNumber, Input signature,
                               const ByteString& issuer, time_t notBefore,
                               time_t notAfter, const ByteString& subject,
                               const SECKEYPublicKey* subjectPublicKey,
                                SECItem const* const* extensions);





SECItem*
CreateEncodedCertificate(PLArenaPool* arena, long version, Input signature,
                         const ByteString& serialNumber,
                         const ByteString& issuerNameDER,
                         time_t notBefore, time_t notAfter,
                         const ByteString& subjectNameDER,
                          SECItem const* const* extensions,
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

  SECItem* tbsCertificate(TBSCertificate(arena, version, serialNumber,
                                         signature, issuerNameDER, notBefore,
                                         notAfter, subjectNameDER,
                                         publicKey.get(), extensions));
  if (!tbsCertificate) {
    return nullptr;
  }

  SECItem*
    result(MaybeLogOutput(SignedData(arena, tbsCertificate,
                                     issuerPrivateKey ? issuerPrivateKey
                                                      : privateKeyTemp.get(),
                                     signatureAlgorithm, false, nullptr),
                          "cert"));
  if (!result) {
    return nullptr;
  }
  privateKeyResult = privateKeyTemp.release();
  return result;
}















static SECItem*
TBSCertificate(PLArenaPool* arena, long versionValue,
               const ByteString& serialNumber, Input signature,
               const ByteString& issuer, time_t notBeforeTime,
               time_t notAfterTime, const ByteString& subject,
               const SECKEYPublicKey* subjectPublicKey,
                SECItem const* const* extensions)
{
  assert(arena);
  assert(subjectPublicKey);
  if (!arena || !subjectPublicKey) {
    return nullptr;
  }

  Output output;

  if (versionValue != static_cast<long>(der::Version::v1)) {
    ByteString versionInteger(Integer(versionValue));
    if (versionInteger == ENCODING_FAILED) {
      return nullptr;
    }
    ByteString version(TLV(der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0,
                           versionInteger));
    if (version == ENCODING_FAILED) {
      return nullptr;
    }
    output.Add(version);
  }

  output.Add(serialNumber);

  SECItem signatureItem = UnsafeMapInputToSECItem(signature);
  if (output.Add(&signatureItem) != Success) {
    return nullptr;
  }

  output.Add(issuer);

  
  
  
  SECItem* validity;
  {
    SECItem* notBefore(TimeToTimeChoice(arena, notBeforeTime));
    if (!notBefore) {
      return nullptr;
    }
    SECItem* notAfter(TimeToTimeChoice(arena, notAfterTime));
    if (!notAfter) {
      return nullptr;
    }
    Output validityOutput;
    if (validityOutput.Add(notBefore) != Success) {
      return nullptr;
    }
    if (validityOutput.Add(notAfter) != Success) {
      return nullptr;
    }
    validity = validityOutput.Squash(arena, der::SEQUENCE);
    if (!validity) {
      return nullptr;
    }
  }
  if (output.Add(validity) != Success) {
    return nullptr;
  }

  output.Add(subject);

  
  
  
  ScopedSECItem subjectPublicKeyInfo(
    SECKEY_EncodeDERSubjectPublicKeyInfo(subjectPublicKey));
  if (!subjectPublicKeyInfo) {
    return nullptr;
  }
  if (output.Add(subjectPublicKeyInfo.get()) != Success) {
    return nullptr;
  }

  if (extensions) {
    Output extensionsOutput;
    while (*extensions) {
      if (extensionsOutput.Add(*extensions) != Success) {
        return nullptr;
      }
      ++extensions;
    }
    SECItem* allExtensions(extensionsOutput.Squash(arena, der::SEQUENCE));
    if (!allExtensions) {
      return nullptr;
    }
    SECItem* extensionsWrapped(
      EncodeNested(arena, der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 3,
                   allExtensions));
    if (!extensions) {
      return nullptr;
    }
    if (output.Add(extensionsWrapped) != Success) {
      return nullptr;
    }
  }

  return output.Squash(arena, der::SEQUENCE);
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




SECItem*
CreateEncodedBasicConstraints(PLArenaPool* arena, bool isCA,
                               long* pathLenConstraintValue,
                              ExtensionCriticality criticality)
{
  assert(arena);
  if (!arena) {
    return nullptr;
  }

  Output value;

  if (isCA) {
    if (value.Add(Boolean(arena, true)) != Success) {
      return nullptr;
    }
  }

  if (pathLenConstraintValue) {
    ByteString pathLenConstraint(Integer(*pathLenConstraintValue));
    if (pathLenConstraint == ENCODING_FAILED) {
      return nullptr;
    }
    value.Add(pathLenConstraint);
  }

  
  static const uint8_t tlv_id_ce_basicConstraints[] = {
    0x06, 0x03, 0x55, 0x1d, 0x13
  };

  return Extension(arena, Input(tlv_id_ce_basicConstraints), criticality, value);
}



SECItem*
CreateEncodedEKUExtension(PLArenaPool* arena, Input ekuOID,
                          ExtensionCriticality criticality)
{
  assert(arena);

  Output value;
  SECItem ekuOIDItem = UnsafeMapInputToSECItem(ekuOID);
  if (value.Add(&ekuOIDItem) != Success) {
    return nullptr;
  }

  
  static const uint8_t tlv_id_ce_extKeyUsage[] = {
    0x06, 0x03, 0x55, 0x1d, 0x25
  };

  return Extension(arena, Input(tlv_id_ce_extKeyUsage), criticality, value);
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

  
  
  

  
  
  
  
  
  
  
  
  
  SECItem* responseStatus = SECITEM_AllocItem(context.arena, nullptr, 3);
  if (!responseStatus) {
    return nullptr;
  }
  responseStatus->data[0] = der::ENUMERATED;
  responseStatus->data[1] = 1;
  responseStatus->data[2] = context.responseStatus;

  SECItem* responseBytesNested = nullptr;
  if (!context.skipResponseBytes) {
    SECItem* responseBytes = ResponseBytes(context);
    if (!responseBytes) {
      return nullptr;
    }

    responseBytesNested = EncodeNested(context.arena,
                                       der::CONSTRUCTED |
                                       der::CONTEXT_SPECIFIC,
                                       responseBytes);
    if (!responseBytesNested) {
      return nullptr;
    }
  }

  Output output;
  if (output.Add(responseStatus) != Success) {
    return nullptr;
  }
  if (responseBytesNested) {
    if (output.Add(responseBytesNested) != Success) {
      return nullptr;
    }
  }
  return MaybeLogOutput(output.Squash(context.arena, der::SEQUENCE), "ocsp");
}




SECItem*
ResponseBytes(OCSPResponseContext& context)
{
  
  static const uint8_t id_pkix_ocsp_basic_encoded[] = {
    0x06, 0x09, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x01
  };
  SECItem id_pkix_ocsp_basic = {
    siBuffer,
    const_cast<uint8_t*>(id_pkix_ocsp_basic_encoded),
    sizeof(id_pkix_ocsp_basic_encoded)
  };
  SECItem* response = BasicOCSPResponse(context);
  if (!response) {
    return nullptr;
  }
  SECItem* responseNested = EncodeNested(context.arena, der::OCTET_STRING,
                                         response);
  if (!responseNested) {
    return nullptr;
  }

  Output output;
  if (output.Add(&id_pkix_ocsp_basic) != Success) {
    return nullptr;
  }
  if (output.Add(responseNested) != Success) {
    return nullptr;
  }
  return output.Squash(context.arena, der::SEQUENCE);
}






SECItem*
BasicOCSPResponse(OCSPResponseContext& context)
{
  SECItem* tbsResponseData = ResponseData(context);
  if (!tbsResponseData) {
    return nullptr;
  }

  
  return SignedData(context.arena, tbsResponseData,
                    context.signerPrivateKey.get(),
                    SignatureAlgorithm::rsa_pkcs1_with_sha256,
                    context.badSignature, context.certs);
}






static SECItem*
OCSPExtension(OCSPResponseContext& context, OCSPResponseExtension* extension)
{
  Output output;
  if (output.Add(&extension->id) != Success) {
    return nullptr;
  }
  if (extension->critical) {
    static const uint8_t trueEncoded[3] = { 0x01, 0x01, 0xFF };
    SECItem critical = {
      siBuffer,
      const_cast<uint8_t*>(trueEncoded),
      sizeof(trueEncoded)
    };
    if (output.Add(&critical) != Success) {
      return nullptr;
    }
  }
  SECItem* value = EncodeNested(context.arena, der::OCTET_STRING,
                                &extension->value);
  if (!value) {
    return nullptr;
  }
  if (output.Add(value) != Success) {
    return nullptr;
  }
  return output.Squash(context.arena, der::SEQUENCE);
}




static SECItem*
Extensions(OCSPResponseContext& context)
{
  Output output;
  for (OCSPResponseExtension* extension = context.extensions;
       extension; extension = extension->next) {
    SECItem* extensionEncoded = OCSPExtension(context, extension);
    if (!extensionEncoded) {
      return nullptr;
    }
    if (output.Add(extensionEncoded) != Success) {
      return nullptr;
    }
  }
  SECItem* extensionsEncoded = output.Squash(context.arena, der::SEQUENCE);
  if (!extensionsEncoded) {
    return nullptr;
  }
  return EncodeNested(context.arena,
                      der::CONSTRUCTED |
                      der::CONTEXT_SPECIFIC |
                      1,
                      extensionsEncoded);
}







SECItem*
ResponseData(OCSPResponseContext& context)
{
  ByteString responderID(ResponderID(context));
  if (responderID == ENCODING_FAILED) {
    return nullptr;
  }
  SECItem* producedAtEncoded = TimeToGeneralizedTime(context.arena,
                                                     context.producedAt);
  if (!producedAtEncoded) {
    return nullptr;
  }
  SECItem* responses = SingleResponse(context);
  if (!responses) {
    return nullptr;
  }
  SECItem* responsesNested = EncodeNested(context.arena, der::SEQUENCE,
                                          responses);
  if (!responsesNested) {
    return nullptr;
  }
  SECItem* responseExtensions = nullptr;
  if (context.extensions || context.includeEmptyExtensions) {
    responseExtensions = Extensions(context);
  }

  Output output;
  output.Add(responderID);
  if (output.Add(producedAtEncoded) != Success) {
    return nullptr;
  }
  if (output.Add(responsesNested) != Success) {
    return nullptr;
  }
  if (responseExtensions) {
    if (output.Add(responseExtensions) != Success) {
      return nullptr;
    }
  }
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







SECItem*
SingleResponse(OCSPResponseContext& context)
{
  SECItem* certID = CertID(context);
  if (!certID) {
    return nullptr;
  }
  SECItem* certStatus = CertStatus(context);
  if (!certStatus) {
    return nullptr;
  }
  SECItem* thisUpdateEncoded = TimeToGeneralizedTime(context.arena,
                                                     context.thisUpdate);
  if (!thisUpdateEncoded) {
    return nullptr;
  }
  SECItem* nextUpdateEncodedNested = nullptr;
  if (context.includeNextUpdate) {
    SECItem* nextUpdateEncoded = TimeToGeneralizedTime(context.arena,
                                                       context.nextUpdate);
    if (!nextUpdateEncoded) {
      return nullptr;
    }
    nextUpdateEncodedNested = EncodeNested(context.arena,
                                           der::CONSTRUCTED |
                                           der::CONTEXT_SPECIFIC |
                                           0,
                                           nextUpdateEncoded);
    if (!nextUpdateEncodedNested) {
      return nullptr;
    }
  }

  Output output;
  if (output.Add(certID) != Success) {
    return nullptr;
  }
  if (output.Add(certStatus) != Success) {
    return nullptr;
  }
  if (output.Add(thisUpdateEncoded) != Success) {
    return nullptr;
  }
  if (nextUpdateEncodedNested) {
    if (output.Add(nextUpdateEncodedNested) != Success) {
      return nullptr;
    }
  }
  return output.Squash(context.arena, der::SEQUENCE);
}






SECItem*
CertID(OCSPResponseContext& context)
{
  SECItem issuerSECItem = UnsafeMapInputToSECItem(context.certID.issuer);
  ByteString issuerNameHash(HashedOctetString(issuerSECItem));
  if (issuerNameHash == ENCODING_FAILED) {
    return nullptr;
  }

  SECItem issuerSubjectPublicKeyInfoSECItem =
    UnsafeMapInputToSECItem(context.certID.issuerSubjectPublicKeyInfo);
  ScopedPtr<CERTSubjectPublicKeyInfo, SECKEY_DestroySubjectPublicKeyInfo>
    spki(SECKEY_DecodeDERSubjectPublicKeyInfo(
           &issuerSubjectPublicKeyInfoSECItem));
  if (!spki) {
    return nullptr;
  }
  ByteString issuerKeyHash(KeyHashHelper(spki.get()));
  if (issuerKeyHash == ENCODING_FAILED) {
    return nullptr;
  }

  ByteString serialNumberValue(context.certID.serialNumber.UnsafeGetData(),
                               context.certID.serialNumber.GetLength());
  ByteString serialNumber(TLV(der::INTEGER, serialNumberValue));
  if (serialNumber == ENCODING_FAILED) {
    return nullptr;
  }

  Output output;

  
  static const uint8_t alg_id_sha1[] = {
    0x30, 0x07, 0x06, 0x05, 0x2b, 0x0e, 0x03, 0x02, 0x1a
  };
  static const SECItem id_sha1 = {
    siBuffer,
    const_cast<uint8_t*>(alg_id_sha1),
    sizeof(alg_id_sha1)
  };

  if (output.Add(&id_sha1) != Success) {
    return nullptr;
  }
  output.Add(issuerNameHash);
  output.Add(issuerKeyHash);
  output.Add(serialNumber);
  return output.Squash(context.arena, der::SEQUENCE);
}












SECItem*
CertStatus(OCSPResponseContext& context)
{
  switch (context.certStatus) {
    
    
    case 0:
    case 2:
    {
      SECItem* status = SECITEM_AllocItem(context.arena, nullptr, 2);
      if (!status) {
        return nullptr;
      }
      status->data[0] = der::CONTEXT_SPECIFIC | context.certStatus;
      status->data[1] = 0;
      return status;
    }
    case 1:
    {
      SECItem* revocationTime = TimeToGeneralizedTime(context.arena,
                                                      context.revocationTime);
      if (!revocationTime) {
        return nullptr;
      }
      
      return EncodeNested(context.arena,
                          der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1,
                          revocationTime);
    }
    default:
      assert(false);
      
  }
  return nullptr;
}

} } } 
