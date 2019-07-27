























#ifndef mozilla_pkix__pkixutil_h
#define mozilla_pkix__pkixutil_h

#include "pkixder.h"

namespace mozilla { namespace pkix {










class BackCert
{
public:
  
  BackCert(InputBuffer certDER, EndEntityOrCA endEntityOrCA,
           const BackCert* childCert)
    : der(certDER)
    , endEntityOrCA(endEntityOrCA)
    , childCert(childCert)
  {
  }

  Result Init();

  const InputBuffer GetDER() const { return der; }
  const der::Version GetVersion() const { return version; }
  const SignedDataWithSignature& GetSignedData() const { return signedData; }
  const InputBuffer GetIssuer() const { return issuer; }
  
  
  
  const InputBuffer GetValidity() const { return validity; }
  const InputBuffer GetSerialNumber() const { return serialNumber; }
  const InputBuffer GetSubject() const { return subject; }
  const InputBuffer GetSubjectPublicKeyInfo() const
  {
    return subjectPublicKeyInfo;
  }
  const InputBuffer* GetAuthorityInfoAccess() const
  {
    return MaybeInputBuffer(authorityInfoAccess);
  }
  const InputBuffer* GetBasicConstraints() const
  {
    return MaybeInputBuffer(basicConstraints);
  }
  const InputBuffer* GetCertificatePolicies() const
  {
    return MaybeInputBuffer(certificatePolicies);
  }
  const InputBuffer* GetExtKeyUsage() const
  {
    return MaybeInputBuffer(extKeyUsage);
  }
  const InputBuffer* GetKeyUsage() const
  {
    return MaybeInputBuffer(keyUsage);
  }
  const InputBuffer* GetInhibitAnyPolicy() const
  {
    return MaybeInputBuffer(inhibitAnyPolicy);
  }
  const InputBuffer* GetNameConstraints() const
  {
    return MaybeInputBuffer(nameConstraints);
  }

private:
  const InputBuffer der;

public:
  const EndEntityOrCA endEntityOrCA;
  BackCert const* const childCert;

private:
  der::Version version;

  
  
  
  
  
  
  static inline const InputBuffer* MaybeInputBuffer(const InputBuffer& item)
  {
    return item.GetLength() > 0 ? &item : nullptr;
  }

  SignedDataWithSignature signedData;
  InputBuffer issuer;
  
  
  
  InputBuffer validity;
  InputBuffer serialNumber;
  InputBuffer subject;
  InputBuffer subjectPublicKeyInfo;

  InputBuffer authorityInfoAccess;
  InputBuffer basicConstraints;
  InputBuffer certificatePolicies;
  InputBuffer extKeyUsage;
  InputBuffer inhibitAnyPolicy;
  InputBuffer keyUsage;
  InputBuffer nameConstraints;
  InputBuffer subjectAltName;

  Result RememberExtension(Input& extnID, const InputBuffer& extnValue,
                            bool& understood);

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

  virtual const InputBuffer* GetDER(size_t i) const
  {
    return i < numItems ? &items[i] : nullptr;
  }

  Result Append(InputBuffer der)
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
  InputBuffer items[MAX_LENGTH]; 
  size_t numItems;

  NonOwningDERArray(const NonOwningDERArray&) ;
  void operator=(const NonOwningDERArray&) ;
};

} } 

#endif 
