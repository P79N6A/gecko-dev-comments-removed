























#ifndef mozilla_pkix__pkixutil_h
#define mozilla_pkix__pkixutil_h

#include "pkixder.h"

namespace mozilla { namespace pkix {










class BackCert
{
public:
  
  BackCert(const SECItem& certDER, EndEntityOrCA endEntityOrCA,
           const BackCert* childCert)
    : der(certDER)
    , endEntityOrCA(endEntityOrCA)
    , childCert(childCert)
  {
  }

  Result Init();

  const SECItem& GetDER() const { return der; }
  const der::Version GetVersion() const { return version; }
  const SignedDataWithSignature& GetSignedData() const { return signedData; }
  const SECItem& GetIssuer() const { return issuer; }
  
  
  
  const SECItem& GetValidity() const { return validity; }
  const SECItem& GetSerialNumber() const { return serialNumber; }
  const SECItem& GetSubject() const { return subject; }
  const SECItem& GetSubjectPublicKeyInfo() const
  {
    return subjectPublicKeyInfo;
  }
  const SECItem* GetAuthorityInfoAccess() const
  {
    return MaybeSECItem(authorityInfoAccess);
  }
  const SECItem* GetBasicConstraints() const
  {
    return MaybeSECItem(basicConstraints);
  }
  const SECItem* GetCertificatePolicies() const
  {
    return MaybeSECItem(certificatePolicies);
  }
  const SECItem* GetExtKeyUsage() const { return MaybeSECItem(extKeyUsage); }
  const SECItem* GetKeyUsage() const { return MaybeSECItem(keyUsage); }
  const SECItem* GetInhibitAnyPolicy() const
  {
    return MaybeSECItem(inhibitAnyPolicy);
  }
  const SECItem* GetNameConstraints() const
  {
    return MaybeSECItem(nameConstraints);
  }

private:
  const SECItem& der;

public:
  const EndEntityOrCA endEntityOrCA;
  BackCert const* const childCert;

private:
  der::Version version;

  
  
  
  
  
  
  static inline const SECItem* MaybeSECItem(const SECItem& item)
  {
    return item.len > 0 ? &item : nullptr;
  }

  
  
  
  struct NonOwningSECItem : public SECItemStr {
    NonOwningSECItem()
    {
      data = nullptr;
      len = 0;
    }
  };

  SignedDataWithSignature signedData;
  NonOwningSECItem issuer;
  
  
  
  NonOwningSECItem validity;
  NonOwningSECItem serialNumber;
  NonOwningSECItem subject;
  NonOwningSECItem subjectPublicKeyInfo;

  NonOwningSECItem authorityInfoAccess;
  NonOwningSECItem basicConstraints;
  NonOwningSECItem certificatePolicies;
  NonOwningSECItem extKeyUsage;
  NonOwningSECItem inhibitAnyPolicy;
  NonOwningSECItem keyUsage;
  NonOwningSECItem nameConstraints;
  NonOwningSECItem subjectAltName;

  Result RememberExtension(Input& extnID, const SECItem& extnValue,
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

  virtual const SECItem* GetDER(size_t i) const
  {
    return i < numItems ? items[i] : nullptr;
  }

  Result Append(const SECItem& der)
  {
    if (numItems >= MAX_LENGTH) {
      return Result::FATAL_ERROR_INVALID_ARGS;
    }
    items[numItems] = &der;
    ++numItems;
    return Success;
  }

  
  static const size_t MAX_LENGTH = 8;
private:
  const SECItem* items[MAX_LENGTH]; 
  size_t numItems;
};

} } 

#endif 
