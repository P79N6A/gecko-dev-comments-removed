























#include "pkix/pkix.h"
#include "pkixder.h"
#include "pkixgtest.h"
#include "pkixtestutil.h"

using namespace mozilla::pkix;
using namespace mozilla::pkix::test;


static ByteString
CreateCertWithExtensions(const char* subjectCN,
                         const ByteString* extensions)
{
  static long serialNumberValue = 0;
  ++serialNumberValue;
  ByteString serialNumber(CreateEncodedSerialNumber(serialNumberValue));
  EXPECT_FALSE(ENCODING_FAILED(serialNumber));
  ByteString issuerDER(CNToDERName(subjectCN));
  EXPECT_FALSE(ENCODING_FAILED(issuerDER));
  ByteString subjectDER(CNToDERName(subjectCN));
  EXPECT_FALSE(ENCODING_FAILED(subjectDER));
  ScopedTestKeyPair subjectKey(CloneReusedKeyPair());
  return CreateEncodedCertificate(v3, sha256WithRSAEncryption,
                                  serialNumber, issuerDER,
                                  oneDayBeforeNow, oneDayAfterNow,
                                  subjectDER, *subjectKey, extensions,
                                  *subjectKey,
                                  sha256WithRSAEncryption);
}


static ByteString
CreateCertWithOneExtension(const char* subjectStr, const ByteString& extension)
{
  const ByteString extensions[] = { extension, ByteString() };
  return CreateCertWithExtensions(subjectStr, extensions);
}

class TrustEverythingTrustDomain final : public TrustDomain
{
private:
  Result GetCertTrust(EndEntityOrCA, const CertPolicyId&, Input,
                       TrustLevel& trustLevel) override
  {
    trustLevel = TrustLevel::TrustAnchor;
    return Success;
  }

  Result FindIssuer(Input , IssuerChecker& ,
                    Time ) override
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  Result CheckRevocation(EndEntityOrCA, const CertID&, Time,
                          const Input*,  const Input*)
                         override
  {
    return Success;
  }

  Result IsChainValid(const DERArray&, Time) override
  {
    return Success;
  }

  Result VerifySignedData(const SignedDataWithSignature& signedData,
                          Input subjectPublicKeyInfo) override
  {
    return TestVerifySignedData(signedData, subjectPublicKeyInfo);
  }

  Result DigestBuf(Input,  uint8_t*, size_t) override
  {
    ADD_FAILURE();
    return Result::FATAL_ERROR_LIBRARY_FAILURE;
  }

  Result CheckRSAPublicKeyModulusSizeInBits(EndEntityOrCA, unsigned int)
                                            override
  {
    return Success;
  }

  Result CheckECDSACurveIsAcceptable(EndEntityOrCA, NamedCurve) override
  {
    return Success;
  }
};


static const uint8_t tlv_unknownExtensionOID[] = {
  0x06, 0x12, 0x2b, 0x06, 0x01, 0x04, 0x01, 0xeb, 0x49, 0x85, 0x1a, 0x85, 0x1a,
  0x85, 0x1a, 0x01, 0x83, 0x74, 0x09, 0x03
};


static const uint8_t tlv_id_pe_authorityInformationAccess[] = {
  0x06, 0x08, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x01, 0x01
};



static const uint8_t tlv_wrongExtensionOID[] = {
  0x06, 0x09, 0x2b, 0x06, 0x06, 0x01, 0x05, 0x05, 0x07, 0x01, 0x01
};




static const uint8_t tlv_id_ce_unknown[] = {
  0x06, 0x03, 0x55, 0x1d, 0x37
};


static const uint8_t tlv_id_ce_inhibitAnyPolicy[] = {
  0x06, 0x03, 0x55, 0x1d, 0x36
};


static const uint8_t tlv_id_pkix_ocsp_nocheck[] = {
  0x06, 0x09, 0x2b, 0x06, 0x01, 0x05, 0x05, 0x07, 0x30, 0x01, 0x05
};

template <size_t L>
inline ByteString
BytesToByteString(const uint8_t (&bytes)[L])
{
  return ByteString(bytes, L);
}

struct ExtensionTestcase
{
  ByteString extension;
  Result expectedResult;
};

static const ExtensionTestcase EXTENSION_TESTCASES[] =
{
  
  
  
  { TLV(der::SEQUENCE,
        BytesToByteString(tlv_unknownExtensionOID) +
        TLV(der::OCTET_STRING, ByteString())),
    Success
  },

  
  
  
  { TLV(der::SEQUENCE,
        BytesToByteString(tlv_unknownExtensionOID) +
        Boolean(true) +
        TLV(der::OCTET_STRING, ByteString())),
    Result::ERROR_UNKNOWN_CRITICAL_EXTENSION
  },

  
  
  
  
  { TLV(der::SEQUENCE,
        BytesToByteString(tlv_id_pe_authorityInformationAccess) +
        Boolean(true) +
        TLV(der::OCTET_STRING, TLV(der::SEQUENCE, ByteString()))),
    Success
  },

  
  
  
  
  { TLV(der::SEQUENCE,
        BytesToByteString(tlv_wrongExtensionOID) +
        Boolean(true) +
        TLV(der::OCTET_STRING, ByteString())),
    Result::ERROR_UNKNOWN_CRITICAL_EXTENSION
  },

  
  
  
  { TLV(der::SEQUENCE,
        BytesToByteString(tlv_id_ce_unknown) +
        Boolean(true) +
        TLV(der::OCTET_STRING, ByteString())),
    Result::ERROR_UNKNOWN_CRITICAL_EXTENSION
  },

  
  
  
  { TLV(der::SEQUENCE,
        BytesToByteString(tlv_id_ce_inhibitAnyPolicy) +
        Boolean(true) +
        TLV(der::OCTET_STRING, Integer(0))),
    Success
  },

  
  
  
  
  
  { TLV(der::SEQUENCE,
        BytesToByteString(tlv_id_pkix_ocsp_nocheck) +
        Boolean(true) +
        TLV(der::OCTET_STRING, TLV(der::NULLTag, ByteString()))),
    Success
  },

  
  
  
  
  
  { TLV(der::SEQUENCE,
        BytesToByteString(tlv_id_pkix_ocsp_nocheck) +
        Boolean(true) +
        TLV(der::OCTET_STRING, ByteString())),
    Success
  },
};

class pkixcert_extension
  : public ::testing::Test
  , public ::testing::WithParamInterface<ExtensionTestcase>
{
protected:
  static TrustEverythingTrustDomain trustDomain;
};

 TrustEverythingTrustDomain pkixcert_extension::trustDomain;

TEST_P(pkixcert_extension, ExtensionHandledProperly)
{
  const ExtensionTestcase& testcase(GetParam());
  const char* cn = "Cert Extension Test";
  ByteString cert(CreateCertWithOneExtension(cn, testcase.extension));
  ASSERT_FALSE(ENCODING_FAILED(cert));
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));
  ASSERT_EQ(testcase.expectedResult,
            BuildCertChain(trustDomain, certInput, Now(),
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::anyExtendedKeyUsage,
                           CertPolicyId::anyPolicy,
                           nullptr));
}

INSTANTIATE_TEST_CASE_P(pkixcert_extension,
                        pkixcert_extension,
                        testing::ValuesIn(EXTENSION_TESTCASES));


TEST_F(pkixcert_extension, DuplicateSubjectAltName)
{
  
  static const uint8_t tlv_id_ce_subjectAltName[] = {
    0x06, 0x03, 0x55, 0x1d, 0x11
  };

  ByteString subjectAltName(
    TLV(der::SEQUENCE,
        BytesToByteString(tlv_id_ce_subjectAltName) +
        TLV(der::OCTET_STRING, TLV(der::SEQUENCE, DNSName("example.com")))));
  static const ByteString extensions[] = { subjectAltName, subjectAltName,
                                           ByteString() };
  static const char* certCN = "Cert With Duplicate subjectAltName";
  ByteString cert(CreateCertWithExtensions(certCN, extensions));
  ASSERT_FALSE(ENCODING_FAILED(cert));
  Input certInput;
  ASSERT_EQ(Success, certInput.Init(cert.data(), cert.length()));
  ASSERT_EQ(Result::ERROR_EXTENSION_VALUE_INVALID,
            BuildCertChain(trustDomain, certInput, Now(),
                           EndEntityOrCA::MustBeEndEntity,
                           KeyUsage::noParticularKeyUsageRequired,
                           KeyPurposeId::anyExtendedKeyUsage,
                           CertPolicyId::anyPolicy,
                           nullptr));
}
