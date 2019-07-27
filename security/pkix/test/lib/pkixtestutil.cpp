























#include "pkixtestutil.h"

#include <cerrno>
#include <cstdio>
#include <limits>
#include <new>

#include "cryptohi.h"
#include "hasht.h"
#include "pk11pub.h"
#include "pkix/pkixnss.h"
#include "pkixder.h"
#include "pkixutil.h"
#include "prinit.h"
#include "prprf.h"
#include "secder.h"
#include "secerr.h"

using namespace std;

namespace mozilla { namespace pkix { namespace test {

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
TamperOnce(SECItem& item,
           const uint8_t* from, size_t fromLen,
           const uint8_t* to, size_t toLen)
{
  if (!item.data || !from || !to || fromLen != toLen) {
    return Result::FATAL_ERROR_INVALID_ARGS;
  }

  if (fromLen < 8) {
    return Result::FATAL_ERROR_INVALID_ARGS;
  }

  uint8_t* p = item.data;
  size_t remaining = item.len;
  bool alreadyFoundMatch = false;
  for (;;) {
    uint8_t* foundFirstByte = static_cast<uint8_t*>(memchr(p, from[0],
                                                           remaining));
    if (!foundFirstByte) {
      if (alreadyFoundMatch) {
        return Success;
      }
      return Result::FATAL_ERROR_INVALID_ARGS;
    }
    remaining -= (foundFirstByte - p);
    if (remaining < fromLen) {
      if (alreadyFoundMatch) {
        return Success;
      }
      return Result::FATAL_ERROR_INVALID_ARGS;
    }
    if (!memcmp(foundFirstByte, from, fromLen)) {
      if (alreadyFoundMatch) {
        return Result::FATAL_ERROR_INVALID_ARGS;
      }
      alreadyFoundMatch = true;
      memmove(foundFirstByte, to, toLen);
      p = foundFirstByte + toLen;
    } else {
      p = foundFirstByte + 1;
      --remaining;
    }
  }
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
    : numItems(0)
    , length(0)
  {
  }

  
  
  Result Add(const SECItem* item)
  {
    assert(item);
    assert(item->data);

    if (numItems >= MaxSequenceItems) {
      return Result::FATAL_ERROR_INVALID_ARGS;
    }
    if (length + item->len > 65535) {
      return Result::FATAL_ERROR_INVALID_ARGS;
    }

    contents[numItems] = item;
    numItems++;
    length += item->len;
    return Success;
  }

  SECItem* Squash(PLArenaPool* arena, uint8_t tag)
  {
    assert(arena);

    size_t lengthLength = length < 128 ? 1
                        : length < 256 ? 2
                                       : 3;
    size_t totalLength = 1 + lengthLength + length;
    SECItem* output = SECITEM_AllocItem(arena, nullptr, totalLength);
    if (!output) {
      return nullptr;
    }
    uint8_t* d = output->data;
    *d++ = tag;
    EncodeLength(d, length, lengthLength);
    d += lengthLength;
    for (size_t i = 0; i < numItems; i++) {
      memcpy(d, contents[i]->data, contents[i]->len);
      d += contents[i]->len;
    }
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

  static const size_t MaxSequenceItems = 10;
  const SECItem* contents[MaxSequenceItems];
  size_t numItems;
  size_t length;

  Output(const Output&) ;
  void operator=(const Output&) ;
};

OCSPResponseContext::OCSPResponseContext(PLArenaPool* arena,
                                         const CertID& certID, time_t time)
  : arena(arena)
  , certID(certID)
  , responseStatus(successful)
  , skipResponseBytes(false)
  , signerNameDER(nullptr)
  , producedAt(time)
  , extensions(nullptr)
  , includeEmptyExtensions(false)
  , badSignature(false)
  , certs(nullptr)

  , certIDHashAlg(SEC_OID_SHA1)
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
static SECItem* ResponderID(OCSPResponseContext& context);
static SECItem* KeyHash(OCSPResponseContext& context);
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



static size_t
HashAlgorithmToLength(SECOidTag hashAlg)
{
  switch (hashAlg) {
    case SEC_OID_SHA1:
      return SHA1_LENGTH;
    case SEC_OID_SHA256:
      return SHA256_LENGTH;
    case SEC_OID_SHA384:
      return SHA384_LENGTH;
    case SEC_OID_SHA512:
      return SHA512_LENGTH;
    default:
      abort();
  }
  return 0;
}

static SECItem*
HashedOctetString(PLArenaPool* arena, const SECItem& bytes, SECOidTag hashAlg)
{
  size_t hashLen = HashAlgorithmToLength(hashAlg);
  if (hashLen == 0) {
    return nullptr;
  }
  SECItem* hashBuf = SECITEM_AllocItem(arena, nullptr, hashLen);
  if (!hashBuf) {
    return nullptr;
  }
  if (PK11_HashBuf(hashAlg, hashBuf->data, bytes.data, bytes.len)
        != SECSuccess) {
    return nullptr;
  }

  return EncodeNested(arena, der::OCTET_STRING, hashBuf);
}

static SECItem*
KeyHashHelper(PLArenaPool* arena, const CERTSubjectPublicKeyInfo* spki)
{
  
  SECItem spk = spki->subjectPublicKey;
  DER_ConvertBitString(&spk); 
  return HashedOctetString(arena, spk, SEC_OID_SHA1);
}

static SECItem*
AlgorithmIdentifier(PLArenaPool* arena, SECOidTag algTag)
{
  SECAlgorithmIDStr aid;
  aid.algorithm.data = nullptr;
  aid.algorithm.len = 0;
  aid.parameters.data = nullptr;
  aid.parameters.len = 0;
  if (SECOID_SetAlgorithmID(arena, &aid, algTag, nullptr) != SECSuccess) {
    return nullptr;
  }
  static const SEC_ASN1Template algorithmIDTemplate[] = {
    { SEC_ASN1_SEQUENCE, 0, NULL, sizeof(SECAlgorithmID) },
    { SEC_ASN1_OBJECT_ID, offsetof(SECAlgorithmID, algorithm) },
    { SEC_ASN1_OPTIONAL | SEC_ASN1_ANY, offsetof(SECAlgorithmID, parameters) },
    { 0 }
  };
  SECItem* algorithmID = SEC_ASN1EncodeItem(arena, nullptr, &aid,
                                            algorithmIDTemplate);
  return algorithmID;
}

static SECItem*
BitString(PLArenaPool* arena, const SECItem* rawBytes, bool corrupt)
{
  
  
  
  SECItem* prefixed = SECITEM_AllocItem(arena, nullptr, rawBytes->len + 1);
  if (!prefixed) {
    return nullptr;
  }
  prefixed->data[0] = 0;
  memcpy(prefixed->data + 1, rawBytes->data, rawBytes->len);
  if (corrupt) {
    assert(prefixed->len > 8);
    prefixed->data[8]++;
  }
  return EncodeNested(arena, der::BIT_STRING, prefixed);
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

static SECItem*
Integer(PLArenaPool* arena, long value)
{
  if (value < 0 || value > 127) {
    
    return nullptr;
  }

  SECItem* encoded = SECITEM_AllocItem(arena, nullptr, 3);
  if (!encoded) {
    return nullptr;
  }
  encoded->data[0] = der::INTEGER;
  encoded->data[1] = 1; 
  encoded->data[2] = value;
  return encoded;
}

static SECItem*
OID(PLArenaPool* arena, SECOidTag tag)
{
  const SECOidData* extnIDData(SECOID_FindOIDByTag(tag));
  if (!extnIDData) {
    return nullptr;
  }
  return EncodeNested(arena, der::OIDTag, &extnIDData->oid);
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
           SECKEYPrivateKey* privKey, SECOidTag hashAlg,
           bool corrupt,  SECItem const* const* certs)
{
  assert(arena);
  assert(tbsData);
  assert(privKey);
  if (!arena || !tbsData || !privKey) {
    return nullptr;
  }

  SECOidTag signatureAlgTag = SEC_GetSignatureAlgorithmOidTag(privKey->keyType,
                                                              hashAlg);
  if (signatureAlgTag == SEC_OID_UNKNOWN) {
    return nullptr;
  }
  SECItem* signatureAlgorithm = AlgorithmIdentifier(arena, signatureAlgTag);
  if (!signatureAlgorithm) {
    return nullptr;
  }

  
  
  SECItem signature;
  if (SEC_SignData(&signature, tbsData->data, tbsData->len, privKey,
                   signatureAlgTag) != SECSuccess)
  {
    return nullptr;
  }
  
  
  SECItem* signatureNested = BitString(arena, &signature, corrupt);
  SECITEM_FreeItem(&signature, false);
  if (!signatureNested) {
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
  if (output.Add(signatureAlgorithm) != Success) {
    return nullptr;
  }
  if (output.Add(signatureNested) != Success) {
    return nullptr;
  }
  if (certsNested) {
    if (output.Add(certsNested) != Success) {
      return nullptr;
    }
  }
  return output.Squash(arena, der::SEQUENCE);
}









static SECItem*
Extension(PLArenaPool* arena, SECOidTag extnIDTag,
          ExtensionCriticality criticality, Output& value)
{
  assert(arena);
  if (!arena) {
    return nullptr;
  }

  Output output;

  const SECItem* extnID(OID(arena, extnIDTag));
  if (!extnID) {
    return nullptr;
  }
  if (output.Add(extnID) != Success) {
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
                               const SECItem* serialNumber, SECOidTag signature,
                               const SECItem* issuer, time_t notBefore,
                               time_t notAfter, const SECItem* subject,
                               const SECKEYPublicKey* subjectPublicKey,
                                SECItem const* const* extensions);





SECItem*
CreateEncodedCertificate(PLArenaPool* arena, long version,
                         SECOidTag signature, const SECItem* serialNumber,
                         const SECItem* issuerNameDER, time_t notBefore,
                         time_t notAfter, const SECItem* subjectNameDER,
                          SECItem const* const* extensions,
                          SECKEYPrivateKey* issuerPrivateKey,
                         SECOidTag signatureHashAlg,
                          ScopedSECKEYPrivateKey& privateKeyResult)
{
  assert(arena);
  assert(issuerNameDER);
  assert(subjectNameDER);
  if (!arena || !issuerNameDER || !subjectNameDER) {
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
                                     signatureHashAlg, false, nullptr),
                          "cert"));
  if (!result) {
    return nullptr;
  }
  privateKeyResult = privateKeyTemp.release();
  return result;
}















static SECItem*
TBSCertificate(PLArenaPool* arena, long versionValue,
               const SECItem* serialNumber, SECOidTag signatureOidTag,
               const SECItem* issuer, time_t notBeforeTime,
               time_t notAfterTime, const SECItem* subject,
               const SECKEYPublicKey* subjectPublicKey,
                SECItem const* const* extensions)
{
  assert(arena);
  assert(issuer);
  assert(subject);
  assert(subjectPublicKey);
  if (!arena || !issuer || !subject || !subjectPublicKey) {
    return nullptr;
  }

  Output output;

  if (versionValue != static_cast<long>(der::Version::v1)) {
    SECItem* versionInteger(Integer(arena, versionValue));
    if (!versionInteger) {
      return nullptr;
    }
    SECItem* version(EncodeNested(arena,
                                  der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0,
                                  versionInteger));
    if (!version) {
      return nullptr;
    }
    if (output.Add(version) != Success) {
      return nullptr;
    }
  }

  if (output.Add(serialNumber) != Success) {
    return nullptr;
  }

  SECItem* signature(AlgorithmIdentifier(arena, signatureOidTag));
  if (!signature) {
    return nullptr;
  }
  if (output.Add(signature) != Success) {
    return nullptr;
  }

  if (output.Add(issuer) != Success) {
    return nullptr;
  }

  
  
  
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

  if (output.Add(subject) != Success) {
    return nullptr;
  }

  
  
  
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

const SECItem*
ASCIIToDERName(PLArenaPool* arena, const char* cn)
{
  ScopedPtr<CERTName, CERT_DestroyName> certName(CERT_AsciiToName(cn));
  if (!certName) {
    return nullptr;
  }
  return SEC_ASN1EncodeItem(arena, nullptr, certName.get(),
                            SEC_ASN1_GET(CERT_NameTemplate));
}

SECItem*
CreateEncodedSerialNumber(PLArenaPool* arena, long serialNumberValue)
{
  return Integer(arena, serialNumberValue);
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
    SECItem* pathLenConstraint(Integer(arena, *pathLenConstraintValue));
    if (!pathLenConstraint) {
      return nullptr;
    }
    if (value.Add(pathLenConstraint) != Success) {
      return nullptr;
    }
  }

  return Extension(arena, SEC_OID_X509_BASIC_CONSTRAINTS, criticality, value);
}



SECItem*
CreateEncodedEKUExtension(PLArenaPool* arena, SECOidTag const* ekus,
                          size_t ekusCount, ExtensionCriticality criticality)
{
  assert(arena);
  assert(ekus);
  if (!arena || (!ekus && ekusCount != 0)) {
    return nullptr;
  }

  Output value;
  for (size_t i = 0; i < ekusCount; ++i) {
    SECItem* encodedEKUOID = OID(arena, ekus[i]);
    if (!encodedEKUOID) {
      return nullptr;
    }
    if (value.Add(encodedEKUOID) != Success) {
      return nullptr;
    }
  }

  return Extension(arena, SEC_OID_X509_EXT_KEY_USAGE, criticality, value);
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
                    context.signerPrivateKey.get(), SEC_OID_SHA1,
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
  SECItem* responderID = ResponderID(context);
  if (!responderID) {
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
  if (output.Add(responderID) != Success) {
    return nullptr;
  }
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





SECItem*
ResponderID(OCSPResponseContext& context)
{
  const SECItem* contents;
  uint8_t responderIDType;
  if (context.signerNameDER) {
    contents = context.signerNameDER;
    responderIDType = 1; 
  } else {
    contents = KeyHash(context);
    responderIDType = 2; 
  }
  if (!contents) {
    return nullptr;
  }

  return EncodeNested(context.arena,
                      der::CONSTRUCTED |
                      der::CONTEXT_SPECIFIC |
                      responderIDType,
                      contents);
}






SECItem*
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
  return KeyHashHelper(context.arena, signerSPKI.get());
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
  SECItem* hashAlgorithm = AlgorithmIdentifier(context.arena,
                                               context.certIDHashAlg);
  if (!hashAlgorithm) {
    return nullptr;
  }
  SECItem issuerSECItem = UnsafeMapInputToSECItem(context.certID.issuer);
  SECItem* issuerNameHash = HashedOctetString(context.arena, issuerSECItem,
                                              context.certIDHashAlg);
  if (!issuerNameHash) {
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
  SECItem* issuerKeyHash(KeyHashHelper(context.arena, spki.get()));
  if (!issuerKeyHash) {
    return nullptr;
  }

  static const SEC_ASN1Template serialTemplate[] = {
    { SEC_ASN1_INTEGER, 0 },
    { 0 }
  };
  SECItem serialNumberSECItem =
    UnsafeMapInputToSECItem(context.certID.serialNumber);
  SECItem* serialNumber = SEC_ASN1EncodeItem(context.arena, nullptr,
                                             &serialNumberSECItem,
                                             serialTemplate);
  if (!serialNumber) {
    return nullptr;
  }

  Output output;
  if (output.Add(hashAlgorithm) != Success) {
    return nullptr;
  }
  if (output.Add(issuerNameHash) != Success) {
    return nullptr;
  }
  if (output.Add(issuerKeyHash) != Success) {
    return nullptr;
  }
  if (output.Add(serialNumber) != Success) {
    return nullptr;
  }
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
