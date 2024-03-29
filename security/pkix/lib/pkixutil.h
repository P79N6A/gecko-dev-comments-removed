























#ifndef mozilla_pkix_pkixutil_h
#define mozilla_pkix_pkixutil_h

#include "pkixder.h"

namespace mozilla { namespace pkix {










class BackCert final
{
public:
  
  BackCert(Input certDER, EndEntityOrCA endEntityOrCA,
           const BackCert* childCert)
    : der(certDER)
    , endEntityOrCA(endEntityOrCA)
    , childCert(childCert)
  {
  }

  Result Init();

  const Input GetDER() const { return der; }
  const der::SignedDataWithSignature& GetSignedData() const {
    return signedData;
  }

  der::Version GetVersion() const { return version; }
  const Input GetSerialNumber() const { return serialNumber; }
  const Input GetSignature() const { return signature; }
  const Input GetIssuer() const { return issuer; }
  
  
  
  const Input GetValidity() const { return validity; }
  const Input GetSubject() const { return subject; }
  const Input GetSubjectPublicKeyInfo() const
  {
    return subjectPublicKeyInfo;
  }
  const Input* GetAuthorityInfoAccess() const
  {
    return MaybeInput(authorityInfoAccess);
  }
  const Input* GetBasicConstraints() const
  {
    return MaybeInput(basicConstraints);
  }
  const Input* GetCertificatePolicies() const
  {
    return MaybeInput(certificatePolicies);
  }
  const Input* GetExtKeyUsage() const
  {
    return MaybeInput(extKeyUsage);
  }
  const Input* GetKeyUsage() const
  {
    return MaybeInput(keyUsage);
  }
  const Input* GetInhibitAnyPolicy() const
  {
    return MaybeInput(inhibitAnyPolicy);
  }
  const Input* GetNameConstraints() const
  {
    return MaybeInput(nameConstraints);
  }
  const Input* GetSubjectAltName() const
  {
    return MaybeInput(subjectAltName);
  }

private:
  const Input der;

public:
  const EndEntityOrCA endEntityOrCA;
  BackCert const* const childCert;

private:
  
  
  
  
  
  
  static inline const Input* MaybeInput(const Input& item)
  {
    return item.GetLength() > 0 ? &item : nullptr;
  }

  der::SignedDataWithSignature signedData;

  der::Version version;
  Input serialNumber;
  Input signature;
  Input issuer;
  
  
  
  Input validity;
  Input subject;
  Input subjectPublicKeyInfo;

  Input authorityInfoAccess;
  Input basicConstraints;
  Input certificatePolicies;
  Input extKeyUsage;
  Input inhibitAnyPolicy;
  Input keyUsage;
  Input nameConstraints;
  Input subjectAltName;
  Input criticalNetscapeCertificateType;

  Result RememberExtension(Reader& extnID, Input extnValue, bool critical,
                            bool& understood);

  BackCert(const BackCert&) = delete;
  void operator=(const BackCert&) = delete;
};

class NonOwningDERArray final : public DERArray
{
public:
  NonOwningDERArray()
    : numItems(0)
  {
    
    
  }

  size_t GetLength() const override { return numItems; }

  const Input* GetDER(size_t i) const override
  {
    return i < numItems ? &items[i] : nullptr;
  }

  Result Append(Input der)
  {
    if (numItems >= MAX_LENGTH) {
      return Result::FATAL_ERROR_INVALID_ARGS;
    }
    Result rv = items[numItems].Init(der); 
    if (rv != Success) {
      return rv;
    }
    ++numItems;
    return Success;
  }

  
  static const size_t MAX_LENGTH = 8;
private:
  Input items[MAX_LENGTH]; 
  size_t numItems;

  NonOwningDERArray(const NonOwningDERArray&) = delete;
  void operator=(const NonOwningDERArray&) = delete;
};

inline unsigned int
DaysBeforeYear(unsigned int year)
{
  assert(year <= 9999);
  return ((year - 1u) * 365u)
       + ((year - 1u) / 4u)    
       - ((year - 1u) / 100u)  
       + ((year - 1u) / 400u); 
}

static const size_t MAX_DIGEST_SIZE_IN_BYTES = 512 / 8; 

Result DigestSignedData(TrustDomain& trustDomain,
                        const der::SignedDataWithSignature& signedData,
                         uint8_t(&digestBuf)[MAX_DIGEST_SIZE_IN_BYTES],
                         der::PublicKeyAlgorithm& publicKeyAlg,
                         SignedDigest& signedDigest);

Result VerifySignedDigest(TrustDomain& trustDomain,
                          der::PublicKeyAlgorithm publicKeyAlg,
                          const SignedDigest& signedDigest,
                          Input signerSubjectPublicKeyInfo);


Result VerifySignedData(TrustDomain& trustDomain,
                        const der::SignedDataWithSignature& signedData,
                        Input signerSubjectPublicKeyInfo);






















#if defined(__clang__)





#define MOZILLA_PKIX_UNREACHABLE_DEFAULT_ENUM
#elif defined(__GNUC__)


#define MOZILLA_PKIX_UNREACHABLE_DEFAULT_ENUM \
        default: assert(false); __builtin_unreachable();
#elif defined(_MSC_VER)


#define MOZILLA_PKIX_UNREACHABLE_DEFAULT_ENUM \
        default: assert(false); __assume(0);
#else
#error Unsupported compiler for MOZILLA_PKIX_UNREACHABLE_DEFAULT.
#endif

} } 

#endif 
