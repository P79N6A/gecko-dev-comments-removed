























#include "pkixtestutil.h"

#include <cerrno>
#include <cstdio>
#include <limits>
#include <new>
#include <sstream>
#include <cstdlib>

#include "pkixder.h"
#include "pkixutil.h"

using namespace std;

namespace mozilla { namespace pkix { namespace test {

namespace {

inline void
fclose_void(FILE* file) {
  (void) fclose(file);
}

typedef mozilla::pkix::ScopedPtr<FILE, fclose_void> ScopedFILE;

FILE*
OpenFile(const string& dir, const string& filename, const string& mode)
{
  string path = dir + '/' + filename;

  ScopedFILE file;
#ifdef _MSC_VER
  {
    FILE* rawFile;
    errno_t error = fopen_s(&rawFile, path.c_str(), mode.c_str());
    if (error) {
      
      rawFile = nullptr;
    }
    file.reset(rawFile);
  }
#else
  file.reset(fopen(path.c_str(), mode.c_str()));
#endif
  return file.release();
}

} 

bool
InputEqualsByteString(Input input, const ByteString& bs)
{
  Input bsInput;
  if (bsInput.Init(bs.data(), bs.length()) != Success) {
    
    
    
    abort();
  }
  return InputsAreEqual(input, bsInput);
}

ByteString
InputToByteString(Input input)
{
  ByteString result;
  Reader reader(input);
  for (;;) {
    uint8_t b;
    if (reader.Read(b) != Success) {
      return result;
    }
    result.push_back(b);
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


ByteString
TLV(uint8_t tag, size_t length, const ByteString& value)
{
  ByteString result;
  result.push_back(tag);

  if (value.length() < 128) {
    result.push_back(static_cast<uint8_t>(length));
  } else if (value.length() < 256) {
    result.push_back(0x81u);
    result.push_back(static_cast<uint8_t>(length));
  } else if (value.length() < 65536) {
    result.push_back(0x82u);
    result.push_back(static_cast<uint8_t>(length / 256));
    result.push_back(static_cast<uint8_t>(length % 256));
  } else {
    
    
    abort();
  }
  result.append(value);
  return result;
}

OCSPResponseContext::OCSPResponseContext(const CertID& certID, time_t time)
  : certID(certID)
  , responseStatus(successful)
  , skipResponseBytes(false)
  , producedAt(time)
  , extensions(nullptr)
  , includeEmptyExtensions(false)
  , signatureAlgorithm(sha256WithRSAEncryption())
  , badSignature(false)
  , certs(nullptr)

  , certStatus(good)
  , revocationTime(0)
  , thisUpdate(time)
  , nextUpdate(time + static_cast<time_t>(Time::ONE_DAY_IN_SECONDS))
  , includeNextUpdate(true)
{
}

static ByteString ResponseBytes(OCSPResponseContext& context);
static ByteString BasicOCSPResponse(OCSPResponseContext& context);
static ByteString ResponseData(OCSPResponseContext& context);
static ByteString ResponderID(OCSPResponseContext& context);
static ByteString KeyHash(const ByteString& subjectPublicKeyInfo);
static ByteString SingleResponse(OCSPResponseContext& context);
static ByteString CertID(OCSPResponseContext& context);
static ByteString CertStatus(OCSPResponseContext& context);

static ByteString
SHA1(const ByteString& toHash)
{
  uint8_t digestBuf[20];
  Input input;
  if (input.Init(toHash.data(), toHash.length()) != Success) {
    abort();
  }
  Result rv = TestDigestBuf(input, DigestAlgorithm::sha1, digestBuf,
                            sizeof(digestBuf));
  if (rv != Success) {
    abort();
  }
  return ByteString(digestBuf, sizeof(digestBuf));
}

static ByteString
HashedOctetString(const ByteString& bytes)
{
  ByteString digest(SHA1(bytes));
  if (ENCODING_FAILED(digest)) {
    return ByteString();
  }
  return TLV(der::OCTET_STRING, digest);
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

ByteString
Boolean(bool value)
{
  ByteString encodedValue;
  encodedValue.push_back(value ? 0xffu : 0x00u);
  return TLV(der::BOOLEAN, encodedValue);
}

ByteString
Integer(long value)
{
  if (value < 0 || value > 127) {
    
    
    
    abort();
  }

  ByteString encodedValue;
  encodedValue.push_back(static_cast<uint8_t>(value));
  return TLV(der::INTEGER, encodedValue);
}

enum TimeEncoding { UTCTime = 0, GeneralizedTime = 1 };


#if defined(WIN32) && !defined(_POSIX_THREAD_SAFE_FUNCTIONS)
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
    return ByteString();
  }

  if (exploded.tm_sec >= 60) {
    
    exploded.tm_sec = 59;
  }

  
  int year = exploded.tm_year + 1900;

  if (encoding == UTCTime && (year < 1950 || year >= 2050)) {
    return ByteString();
  }

  ByteString value;

  if (encoding == GeneralizedTime) {
    value.push_back(static_cast<uint8_t>('0' + (year / 1000)));
    value.push_back(static_cast<uint8_t>('0' + ((year % 1000) / 100)));
  }

  value.push_back(static_cast<uint8_t>('0' + ((year % 100) / 10)));
  value.push_back(static_cast<uint8_t>('0' + (year % 10)));
  value.push_back(static_cast<uint8_t>('0' + ((exploded.tm_mon + 1) / 10)));
  value.push_back(static_cast<uint8_t>('0' + ((exploded.tm_mon + 1) % 10)));
  value.push_back(static_cast<uint8_t>('0' + (exploded.tm_mday / 10)));
  value.push_back(static_cast<uint8_t>('0' + (exploded.tm_mday % 10)));
  value.push_back(static_cast<uint8_t>('0' + (exploded.tm_hour / 10)));
  value.push_back(static_cast<uint8_t>('0' + (exploded.tm_hour % 10)));
  value.push_back(static_cast<uint8_t>('0' + (exploded.tm_min / 10)));
  value.push_back(static_cast<uint8_t>('0' + (exploded.tm_min % 10)));
  value.push_back(static_cast<uint8_t>('0' + (exploded.tm_sec / 10)));
  value.push_back(static_cast<uint8_t>('0' + (exploded.tm_sec % 10)));
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
    return ByteString();
  }
  TimeEncoding encoding = (exploded.tm_year + 1900 >= 1950 &&
                           exploded.tm_year + 1900 < 2050)
                        ? UTCTime
                        : GeneralizedTime;

  return TimeToEncodedTime(time, encoding);
}

Time
YMDHMS(uint16_t year, uint16_t month, uint16_t day,
       uint16_t hour, uint16_t minutes, uint16_t seconds)
{
  assert(year <= 9999);
  assert(month >= 1);
  assert(month <= 12);
  assert(day >= 1);
  assert(hour < 24);
  assert(minutes < 60);
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
           const TestKeyPair& keyPair,
           const TestSignatureAlgorithm& signatureAlgorithm,
           bool corrupt,  const ByteString* certs)
{
  ByteString signature;
  if (keyPair.SignData(tbsData, signatureAlgorithm, signature) != Success) {
    return ByteString();
  }

  
  
  ByteString signatureNested(BitString(signature, corrupt));
  if (ENCODING_FAILED(signatureNested)) {
    return ByteString();
  }

  ByteString certsNested;
  if (certs) {
    ByteString certsSequenceValue;
    while (!(*certs).empty()) {
      certsSequenceValue.append(*certs);
      ++certs;
    }
    ByteString certsSequence(TLV(der::SEQUENCE, certsSequenceValue));
    certsNested = TLV(der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0,
                      certsSequence);
  }

  ByteString value;
  value.append(tbsData);
  value.append(signatureAlgorithm.algorithmIdentifier);
  value.append(signatureNested);
  value.append(certsNested);
  return TLV(der::SEQUENCE, value);
}









static ByteString
Extension(Input extnID, Critical critical, const ByteString& extnValueBytes)
{
  ByteString encoded;

  encoded.append(ByteString(extnID.UnsafeGetData(), extnID.GetLength()));

  if (critical == Critical::Yes) {
    encoded.append(Boolean(true));
  }

  ByteString extnValueSequence(TLV(der::SEQUENCE, extnValueBytes));
  ByteString extnValue(TLV(der::OCTET_STRING, extnValueSequence));
  encoded.append(extnValue);
  return TLV(der::SEQUENCE, encoded);
}

static ByteString
EmptyExtension(Input extnID, Critical critical)
{
  ByteString encoded(extnID.UnsafeGetData(), extnID.GetLength());

  if (critical == Critical::Yes) {
    encoded.append(Boolean(true));
  }

  ByteString extnValue(TLV(der::OCTET_STRING, ByteString()));
  encoded.append(extnValue);
  return TLV(der::SEQUENCE, encoded);
}

std::string
GetEnv(const char* name)
{
  std::string result;

#ifndef _MSC_VER
  
  const char* value = getenv(name);
  if (value) {
    result = value;
  }
#else
  char* value = nullptr;
  size_t valueLength = 0;
  if (_dupenv_s(&value, &valueLength, name) != 0) {
    abort();
  }
  if (value) {
    result = value;
    free(value);
  }
#endif
  return result;
}

void
MaybeLogOutput(const ByteString& result, const char* suffix)
{
  assert(suffix);

  
  
  
  std::string logPath(GetEnv("MOZILLA_PKIX_TEST_LOG_DIR"));
  if (!logPath.empty()) {
    static int counter = 0;

    std::ostringstream counterStream;
    counterStream << counter;
    if (!counterStream) {
      assert(false);
      return;
    }
    string filename = counterStream.str() + '-' + suffix + ".der";

    ++counter;
    ScopedFILE file(OpenFile(logPath, filename, "wb"));
    if (file) {
      (void) fwrite(result.data(), result.length(), 1, file.get());
    }
  }
}




static ByteString TBSCertificate(long version, const ByteString& serialNumber,
                                 const ByteString& signature,
                                 const ByteString& issuer,
                                 time_t notBefore, time_t notAfter,
                                 const ByteString& subject,
                                 const ByteString& subjectPublicKeyInfo,
                                  const ByteString* extensions);





ByteString
CreateEncodedCertificate(long version,
                         const TestSignatureAlgorithm& signature,
                         const ByteString& serialNumber,
                         const ByteString& issuerNameDER,
                         time_t notBefore, time_t notAfter,
                         const ByteString& subjectNameDER,
                         const TestKeyPair& subjectKeyPair,
                          const ByteString* extensions,
                         const TestKeyPair& issuerKeyPair,
                         const TestSignatureAlgorithm& signatureAlgorithm)
{
  ByteString tbsCertificate(TBSCertificate(version, serialNumber,
                                           signature.algorithmIdentifier,
                                           issuerNameDER, notBefore,
                                           notAfter, subjectNameDER,
                                           subjectKeyPair.subjectPublicKeyInfo,
                                           extensions));
  if (ENCODING_FAILED(tbsCertificate)) {
    return ByteString();
  }

  ByteString result(SignedData(tbsCertificate, issuerKeyPair,
                               signatureAlgorithm, false, nullptr));
  if (ENCODING_FAILED(result)) {
    return ByteString();
  }

  MaybeLogOutput(result, "cert");

  return result;
}















static ByteString
TBSCertificate(long versionValue,
               const ByteString& serialNumber, const ByteString& signature,
               const ByteString& issuer, time_t notBeforeTime,
               time_t notAfterTime, const ByteString& subject,
               const ByteString& subjectPublicKeyInfo,
                const ByteString* extensions)
{
  ByteString value;

  if (versionValue != static_cast<long>(der::Version::v1)) {
    ByteString versionInteger(Integer(versionValue));
    ByteString version(TLV(der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 0,
                           versionInteger));
    value.append(version);
  }

  value.append(serialNumber);
  value.append(signature);
  value.append(issuer);

  
  
  
  ByteString validity;
  {
    ByteString notBefore(TimeToTimeChoice(notBeforeTime));
    if (ENCODING_FAILED(notBefore)) {
      return ByteString();
    }
    ByteString notAfter(TimeToTimeChoice(notAfterTime));
    if (ENCODING_FAILED(notAfter)) {
      return ByteString();
    }
    ByteString validityValue;
    validityValue.append(notBefore);
    validityValue.append(notAfter);
    validity = TLV(der::SEQUENCE, validityValue);
    if (ENCODING_FAILED(validity)) {
      return ByteString();
    }
  }
  value.append(validity);

  value.append(subject);

  value.append(subjectPublicKeyInfo);

  if (extensions) {
    ByteString extensionsValue;
    while (!(*extensions).empty()) {
      extensionsValue.append(*extensions);
      ++extensions;
    }
    ByteString extensionsSequence(TLV(der::SEQUENCE, extensionsValue));
    if (ENCODING_FAILED(extensionsSequence)) {
      return ByteString();
    }
    ByteString extensionsWrapped(
      TLV(der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 3, extensionsSequence));
    if (ENCODING_FAILED(extensionsWrapped)) {
      return ByteString();
    }
    value.append(extensionsWrapped);
  }

  return TLV(der::SEQUENCE, value);
}















template <size_t N>
static ByteString
AVA(const uint8_t (&type)[N], uint8_t directoryStringType,
    const ByteString& value)
{
  ByteString wrappedValue(TLV(directoryStringType, value));
  ByteString ava;
  ava.append(type, N);
  ava.append(wrappedValue);
  return TLV(der::SEQUENCE, ava);
}

ByteString
CN(const ByteString& value, uint8_t encodingTag)
{
  
  
  
  static const uint8_t tlv_id_at_commonName[] = {
    0x06, 0x03, 0x55, 0x04, 0x03
  };
  return AVA(tlv_id_at_commonName, encodingTag, value);
}

ByteString
OU(const ByteString& value, uint8_t encodingTag)
{
  
  
  
  static const uint8_t tlv_id_at_organizationalUnitName[] = {
    0x06, 0x03, 0x55, 0x04, 0x0b
  };

  return AVA(tlv_id_at_organizationalUnitName, encodingTag, value);
}

ByteString
emailAddress(const ByteString& value)
{
  
  
  static const uint8_t tlv_id_emailAddress[] = {
    0x06, 0x09, 0x2a, 0x86, 0x48, 0x86, 0xf7, 0x0d, 0x01, 0x09, 0x01
  };

  return AVA(tlv_id_emailAddress, der::IA5String, value);
}




ByteString
RDN(const ByteString& avas)
{
  return TLV(der::SET, avas);
}






ByteString
Name(const ByteString& rdns)
{
  return TLV(der::SEQUENCE, rdns);
}

ByteString
CreateEncodedSerialNumber(long serialNumberValue)
{
  return Integer(serialNumberValue);
}




ByteString
CreateEncodedBasicConstraints(bool isCA,
                               long* pathLenConstraintValue,
                              Critical critical)
{
  ByteString value;

  if (isCA) {
    ByteString cA(Boolean(true));
    value.append(cA);
  }

  if (pathLenConstraintValue) {
    ByteString pathLenConstraint(Integer(*pathLenConstraintValue));
    value.append(pathLenConstraint);
  }

  
  static const uint8_t tlv_id_ce_basicConstraints[] = {
    0x06, 0x03, 0x55, 0x1d, 0x13
  };
  return Extension(Input(tlv_id_ce_basicConstraints), critical, value);
}



ByteString
CreateEncodedEKUExtension(Input ekuOID, Critical critical)
{
  ByteString value(ekuOID.UnsafeGetData(), ekuOID.GetLength());

  
  static const uint8_t tlv_id_ce_extKeyUsage[] = {
    0x06, 0x03, 0x55, 0x1d, 0x25
  };

  return Extension(Input(tlv_id_ce_extKeyUsage), critical, value);
}


static const uint8_t tlv_id_ce_subjectAltName[] = {
  0x06, 0x03, 0x55, 0x1d, 0x11
};

ByteString
CreateEncodedSubjectAltName(const ByteString& names)
{
  return Extension(Input(tlv_id_ce_subjectAltName), Critical::No, names);
}

ByteString
CreateEncodedEmptySubjectAltName()
{
  return EmptyExtension(Input(tlv_id_ce_subjectAltName), Critical::No);
}




ByteString
CreateEncodedOCSPResponse(OCSPResponseContext& context)
{
  if (!context.skipResponseBytes) {
    if (!context.signerKeyPair) {
      return ByteString();
    }
  }

  
  
  

  
  
  
  
  
  
  
  
  
  ByteString reponseStatusValue;
  reponseStatusValue.push_back(context.responseStatus);
  ByteString responseStatus(TLV(der::ENUMERATED, reponseStatusValue));

  ByteString responseBytesNested;
  if (!context.skipResponseBytes) {
    ByteString responseBytes(ResponseBytes(context));
    if (ENCODING_FAILED(responseBytes)) {
      return ByteString();
    }

    responseBytesNested = TLV(der::CONSTRUCTED | der::CONTEXT_SPECIFIC,
                              responseBytes);
  }

  ByteString value;
  value.append(responseStatus);
  value.append(responseBytesNested);
  ByteString result(TLV(der::SEQUENCE, value));

  MaybeLogOutput(result, "ocsp");

  return result;
}




ByteString
ResponseBytes(OCSPResponseContext& context)
{
  
  static const uint8_t id_pkix_ocsp_basic_encoded[] = {
    0x06, 0x09, 0x2B, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x01
  };
  ByteString response(BasicOCSPResponse(context));
  if (ENCODING_FAILED(response)) {
    return ByteString();
  }
  ByteString responseNested = TLV(der::OCTET_STRING, response);

  ByteString value;
  value.append(id_pkix_ocsp_basic_encoded,
               sizeof(id_pkix_ocsp_basic_encoded));
  value.append(responseNested);
  return TLV(der::SEQUENCE, value);
}






ByteString
BasicOCSPResponse(OCSPResponseContext& context)
{
  ByteString tbsResponseData(ResponseData(context));
  if (ENCODING_FAILED(tbsResponseData)) {
    return ByteString();
  }

  return SignedData(tbsResponseData, *context.signerKeyPair,
                    context.signatureAlgorithm, context.badSignature,
                    context.certs);
}






static ByteString
OCSPExtension(OCSPResponseExtension& extension)
{
  ByteString encoded;
  encoded.append(extension.id);
  if (extension.critical) {
    encoded.append(Boolean(true));
  }
  ByteString value(TLV(der::OCTET_STRING, extension.value));
  encoded.append(value);
  return TLV(der::SEQUENCE, encoded);
}




static ByteString
Extensions(OCSPResponseContext& context)
{
  ByteString value;
  for (OCSPResponseExtension* extension = context.extensions;
       extension; extension = extension->next) {
    ByteString extensionEncoded(OCSPExtension(*extension));
    if (ENCODING_FAILED(extensionEncoded)) {
      return ByteString();
    }
    value.append(extensionEncoded);
  }
  ByteString sequence(TLV(der::SEQUENCE, value));
  return TLV(der::CONSTRUCTED | der::CONTEXT_SPECIFIC | 1, sequence);
}







ByteString
ResponseData(OCSPResponseContext& context)
{
  ByteString responderID(ResponderID(context));
  if (ENCODING_FAILED(responderID)) {
    return ByteString();
  }
  ByteString producedAtEncoded(TimeToGeneralizedTime(context.producedAt));
  if (ENCODING_FAILED(producedAtEncoded)) {
    return ByteString();
  }
  ByteString response(SingleResponse(context));
  if (ENCODING_FAILED(response)) {
    return ByteString();
  }
  ByteString responses(TLV(der::SEQUENCE, response));
  ByteString responseExtensions;
  if (context.extensions || context.includeEmptyExtensions) {
    responseExtensions = Extensions(context);
  }

  ByteString value;
  value.append(responderID);
  value.append(producedAtEncoded);
  value.append(responses);
  value.append(responseExtensions);
  return TLV(der::SEQUENCE, value);
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
    contents = KeyHash(context.signerKeyPair->subjectPublicKey);
    if (ENCODING_FAILED(contents)) {
      return ByteString();
    }
    responderIDType = 2; 
  }

  
  
  uint8_t tag = static_cast<uint8_t>(der::CONSTRUCTED | der::CONTEXT_SPECIFIC |
                                     responderIDType);
  return TLV(tag, contents);
}






ByteString
KeyHash(const ByteString& subjectPublicKey)
{
  return HashedOctetString(subjectPublicKey);
}







ByteString
SingleResponse(OCSPResponseContext& context)
{
  ByteString certID(CertID(context));
  if (ENCODING_FAILED(certID)) {
    return ByteString();
  }
  ByteString certStatus(CertStatus(context));
  if (ENCODING_FAILED(certStatus)) {
    return ByteString();
  }
  ByteString thisUpdateEncoded(TimeToGeneralizedTime(context.thisUpdate));
  if (ENCODING_FAILED(thisUpdateEncoded)) {
    return ByteString();
  }
  ByteString nextUpdateEncodedNested;
  if (context.includeNextUpdate) {
    ByteString nextUpdateEncoded(TimeToGeneralizedTime(context.nextUpdate));
    if (ENCODING_FAILED(nextUpdateEncoded)) {
      return ByteString();
    }
    nextUpdateEncodedNested = TLV(der::CONSTRUCTED | der::CONTEXT_SPECIFIC | 0,
                                  nextUpdateEncoded);
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
  ByteString issuerName(context.certID.issuer.UnsafeGetData(),
                        context.certID.issuer.GetLength());
  ByteString issuerNameHash(HashedOctetString(issuerName));
  if (ENCODING_FAILED(issuerNameHash)) {
    return ByteString();
  }

  ByteString issuerKeyHash;
  {
    
    
    
    Reader input(context.certID.issuerSubjectPublicKeyInfo);
    Reader contents;
    if (der::ExpectTagAndGetValue(input, der::SEQUENCE, contents) != Success) {
      return ByteString();
    }
    
    if (der::ExpectTagAndSkipValue(contents, der::SEQUENCE) != Success) {
      return ByteString();
    }
    Input subjectPublicKey;
    if (der::BitStringWithNoUnusedBits(contents, subjectPublicKey)
          != Success) {
      return ByteString();
    }
    issuerKeyHash = KeyHash(ByteString(subjectPublicKey.UnsafeGetData(),
                                       subjectPublicKey.GetLength()));
    if (ENCODING_FAILED(issuerKeyHash)) {
      return ByteString();
    }
  }

  ByteString serialNumberValue(context.certID.serialNumber.UnsafeGetData(),
                               context.certID.serialNumber.GetLength());
  ByteString serialNumber(TLV(der::INTEGER, serialNumberValue));

  
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
      
      
      return TLV(static_cast<uint8_t>(der::CONTEXT_SPECIFIC |
                                      context.certStatus), ByteString());
    }
    case 1:
    {
      ByteString revocationTime(TimeToGeneralizedTime(context.revocationTime));
      if (ENCODING_FAILED(revocationTime)) {
        return ByteString();
      }
      
      return TLV(der::CONTEXT_SPECIFIC | der::CONSTRUCTED | 1, revocationTime);
    }
    default:
      assert(false);
      
  }
  return ByteString();
}

static const ByteString NO_UNUSED_BITS(1, 0x00);


TestKeyPair::TestKeyPair(const TestPublicKeyAlgorithm& publicKeyAlg,
                         const ByteString& spk)
  : publicKeyAlg(publicKeyAlg)
  , subjectPublicKeyInfo(TLV(der::SEQUENCE,
                             publicKeyAlg.algorithmIdentifier +
                             TLV(der::BIT_STRING, NO_UNUSED_BITS + spk)))
  , subjectPublicKey(spk)
{
}

} } } 
