























#ifndef mozilla_pkix__pkixutil_h
#define mozilla_pkix__pkixutil_h

#include "pkixder.h"

namespace mozilla { namespace pkix {










class BackCert
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
  const der::Version GetVersion() const { return version; }
  const SignedDataWithSignature& GetSignedData() const { return signedData; }
  const Input GetIssuer() const { return issuer; }
  
  
  
  const Input GetValidity() const { return validity; }
  const Input GetSerialNumber() const { return serialNumber; }
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

private:
  const Input der;

public:
  const EndEntityOrCA endEntityOrCA;
  BackCert const* const childCert;

private:
  der::Version version;

  
  
  
  
  
  
  static inline const Input* MaybeInput(const Input& item)
  {
    return item.GetLength() > 0 ? &item : nullptr;
  }

  SignedDataWithSignature signedData;
  Input issuer;
  
  
  
  Input validity;
  Input serialNumber;
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

  Result RememberExtension(Reader& extnID, const Input& extnValue,
                           bool critical,  bool& understood);

  BackCert(const BackCert&) ;
  void operator=(const BackCert&); ;
};

class NonOwningDERArray : public DERArray
{
public:
  NonOwningDERArray()
    : numItems(0)
  {
    
    
  }

  virtual size_t GetLength() const { return numItems; }

  virtual const Input* GetDER(size_t i) const
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

  NonOwningDERArray(const NonOwningDERArray&) ;
  void operator=(const NonOwningDERArray&) ;
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

} } 

#endif 
