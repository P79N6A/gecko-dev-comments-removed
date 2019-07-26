







#include <stdint.h>

static const char kBaltimore_CyberTrust_RootFingerprint[]=
  "Y9mvm0exBk1JoQ57f9Vm28jKo5lFm/woKcVxrYxu80o=";


static const char kDigiCert_Assured_ID_Root_CAFingerprint[]=
  "I/Lt/z7ekCWanjD0Cvj5EqXls2lOaThEA0H2Bg4BT/o=";


static const char kDigiCert_Global_Root_CAFingerprint[]=
  "r/mIkG3eEpVdm+u/ko/cwxzOMo1bk4TyHIlByibiA5E=";


static const char kDigiCert_High_Assurance_EV_Root_CAFingerprint[]=
  "WoiWRyIOVNa9ihaBciRSC7XHjliYS9VwUGOIud4PB18=";


static const char kEnd_Entity_Test_CertFingerprint[]=
  "sEIYDccDj1ULE64YxhvqV7ASqc2qfIofVyArzg+62hU=";


static const char kEquifax_Secure_CAFingerprint[]=
  "/1aAzXOlcD2gSBegdf1GJQanNQbEuBoVg+9UlHjSZHY=";


static const char kGTE_CyberTrust_Global_RootFingerprint[]=
  "EGn6R6CqT4z3ERscrqNl7q7RC//zJmDe9uBhS/rnCHU=";


static const char kGeoTrust_Global_CAFingerprint[]=
  "h6801m+z8v3zbgkRHpq6L29Esgfzhj89C1SyUCOQmqU=";


static const char kGeoTrust_Global_CA_2Fingerprint[]=
  "F3VaXClfPS1y5vAxofB/QAxYi55YKyLxfq4xoVkNEYU=";


static const char kGeoTrust_Primary_Certification_AuthorityFingerprint[]=
  "SQVGZiOrQXi+kqxcvWWE96HhfydlLVqFr4lQTqI5qqo=";


static const char kGeoTrust_Primary_Certification_Authority___G2Fingerprint[]=
  "vPtEqrmtAhAVcGtBIep2HIHJ6IlnWQ9vlK50TciLePs=";


static const char kGeoTrust_Primary_Certification_Authority___G3Fingerprint[]=
  "q5hJUnat8eyv8o81xTBIeB5cFxjaucjmelBPT2pRMo8=";


static const char kGeoTrust_Universal_CAFingerprint[]=
  "lpkiXF3lLlbN0y3y6W0c/qWqPKC7Us2JM8I7XCdEOCA=";


static const char kGeoTrust_Universal_CA_2Fingerprint[]=
  "fKoDRlEkWQxgHlZ+UhSOlSwM/+iQAFMP4NlbbVDqrkE=";


static const char kVeriSign_Class_3_Public_Primary_Certification_Authority___G4Fingerprint[]=
  "UZJDjsNp1+4M5x9cbbdflB779y5YRBcV6Z6rBMLIrO4=";


static const char kVeriSign_Class_3_Public_Primary_Certification_Authority___G5Fingerprint[]=
  "JbQbUG5JMJUoI6brnx0x3vZF6jilxsapbXGVfjhN8Fg=";


static const char kVeriSign_Universal_Root_Certification_AuthorityFingerprint[]=
  "lnsM2T/O9/J84sJFdnrpsFp3awZJ+ZZbYpCWhGloaHI=";


static const char kVerisign_Class_1_Public_Primary_Certification_AuthorityFingerprint[]=
  "LclHC+Y+9KzxvYKGCUArt7h72ZY4pkOTTohoLRvowwg=";


static const char kVerisign_Class_1_Public_Primary_Certification_Authority___G2Fingerprint[]=
  "FqngEtMjKfKCsQu/V8fAtCroD2rJVC60CbwcLN5Q0yI=";


static const char kVerisign_Class_1_Public_Primary_Certification_Authority___G3Fingerprint[]=
  "IgduWu9Eu5pBaii30cRDItcFn2D+/6XK9sW+hEeJEwM=";


static const char kVerisign_Class_2_Public_Primary_Certification_Authority___G2Fingerprint[]=
  "2oALgLKofTmeZvoZ1y/fSZg7R9jPMix8eVA6DH4o/q8=";


static const char kVerisign_Class_2_Public_Primary_Certification_Authority___G3Fingerprint[]=
  "cAajgxHlj7GTSEIzIYIQxmEloOSoJq7VOaxWHfv72QM=";


static const char kVerisign_Class_3_Public_Primary_Certification_AuthorityFingerprint[]=
  "sRJBQqWhpaKIGcc1NA7/jJ4vgWj+47oYfyU7waOS1+I=";


static const char kVerisign_Class_3_Public_Primary_Certification_Authority___G2Fingerprint[]=
  "AjyBzOjnxk+pQtPBUEhwfTXZu1uH9PVExb8bxWQ68vo=";


static const char kVerisign_Class_3_Public_Primary_Certification_Authority___G3Fingerprint[]=
  "SVqWumuteCQHvVIaALrOZXuzVVVeS7f4FGxxu6V+es4=";


static const char kVerisign_Class_4_Public_Primary_Certification_Authority___G3Fingerprint[]=
  "VnuCEf0g09KD7gzXzgZyy52ZvFtIeljJ1U7Gf3fUqPU=";


static const char kthawte_Primary_Root_CAFingerprint[]=
  "HXXQgxueCIU5TTLHob/bPbwcKOKw6DkfsTWYHbxbqTY=";


static const char kthawte_Primary_Root_CA___G2Fingerprint[]=
  "Z9xPMvoQ59AaeaBzqgyeAhLsL/w9d54Kp/nA8OHCyJM=";


static const char kthawte_Primary_Root_CA___G3Fingerprint[]=
  "GQbGEk27Q4V40A4GbVBUxsN/D6YCjAVUXgmU7drshik=";


typedef struct {
  const size_t size;
  const char* const* data;
} StaticPinset;
static const char* const kPinSet_mozilla_Data[] = {
    kEquifax_Secure_CAFingerprint,
    kGeoTrust_Global_CA_2Fingerprint,
    kthawte_Primary_Root_CA___G3Fingerprint,
    kthawte_Primary_Root_CAFingerprint,
    kDigiCert_Assured_ID_Root_CAFingerprint,
    kGeoTrust_Primary_Certification_AuthorityFingerprint,
    kDigiCert_High_Assurance_EV_Root_CAFingerprint,
    kthawte_Primary_Root_CA___G2Fingerprint,
    kGeoTrust_Universal_CA_2Fingerprint,
    kGeoTrust_Global_CAFingerprint,
    kGeoTrust_Universal_CAFingerprint,
    kGeoTrust_Primary_Certification_Authority___G3Fingerprint,
    kDigiCert_Global_Root_CAFingerprint,
    kGeoTrust_Primary_Certification_Authority___G2Fingerprint,
};
const StaticPinset kPinSet_mozilla = { 14, kPinSet_mozilla_Data};

static const char* const kPinSet_mozilla_cdn_Data[] = {
    kEquifax_Secure_CAFingerprint,
    kVerisign_Class_2_Public_Primary_Certification_Authority___G2Fingerprint,
    kVerisign_Class_3_Public_Primary_Certification_Authority___G2Fingerprint,
    kGTE_CyberTrust_Global_RootFingerprint,
    kGeoTrust_Global_CA_2Fingerprint,
    kVerisign_Class_1_Public_Primary_Certification_Authority___G2Fingerprint,
    kthawte_Primary_Root_CA___G3Fingerprint,
    kthawte_Primary_Root_CAFingerprint,
    kDigiCert_Assured_ID_Root_CAFingerprint,
    kVerisign_Class_1_Public_Primary_Certification_Authority___G3Fingerprint,
    kVeriSign_Class_3_Public_Primary_Certification_Authority___G5Fingerprint,
    kVerisign_Class_1_Public_Primary_Certification_AuthorityFingerprint,
    kGeoTrust_Primary_Certification_AuthorityFingerprint,
    kVerisign_Class_3_Public_Primary_Certification_Authority___G3Fingerprint,
    kVeriSign_Class_3_Public_Primary_Certification_Authority___G4Fingerprint,
    kVerisign_Class_4_Public_Primary_Certification_Authority___G3Fingerprint,
    kDigiCert_High_Assurance_EV_Root_CAFingerprint,
    kBaltimore_CyberTrust_RootFingerprint,
    kthawte_Primary_Root_CA___G2Fingerprint,
    kVerisign_Class_2_Public_Primary_Certification_Authority___G3Fingerprint,
    kGeoTrust_Universal_CA_2Fingerprint,
    kGeoTrust_Global_CAFingerprint,
    kVeriSign_Universal_Root_Certification_AuthorityFingerprint,
    kGeoTrust_Universal_CAFingerprint,
    kGeoTrust_Primary_Certification_Authority___G3Fingerprint,
    kDigiCert_Global_Root_CAFingerprint,
    kVerisign_Class_3_Public_Primary_Certification_AuthorityFingerprint,
    kGeoTrust_Primary_Certification_Authority___G2Fingerprint,
};
const StaticPinset kPinSet_mozilla_cdn = { 28, kPinSet_mozilla_cdn_Data};

static const char* const kPinSet_mozilla_test_Data[] = {
    kEnd_Entity_Test_CertFingerprint,
};
const StaticPinset kPinSet_mozilla_test = { 1, kPinSet_mozilla_test_Data};


typedef struct {
  const char *mHost;
  const bool mIncludeSubdomains;
  const StaticPinset *pinset;
} TransportSecurityPreload;

static const TransportSecurityPreload kPublicKeyPinningPreloadList[] = {
  { "exclude-subdomains.pinning.example.com",	false,	&kPinSet_mozilla_test },
  { "include-subdomains.pinning.example.com",	true,	&kPinSet_mozilla_test },
};

static const int kPublicKeyPinningPreloadListLength = 2;

const PRTime kPreloadPKPinsExpirationTime = INT64_C(1410109244157000);
