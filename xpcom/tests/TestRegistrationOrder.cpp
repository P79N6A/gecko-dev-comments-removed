





#include "TestHarness.h"
#include "nsIFile.h"
#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsCOMArray.h"
#include "nsArrayEnumerator.h"
#include "nsXULAppAPI.h"
#include "nsIComponentRegistrar.h"

#define SERVICE_A_CONTRACT_ID  "@mozilla.org/RegTestServiceA;1"
#define SERVICE_B_CONTRACT_ID  "@mozilla.org/RegTestServiceB;1"


#define CORE_SERVICE_A_CID             \
        { 0x56ab1cd4, 0xac44, 0x4f86, \
        { 0x81, 0x04, 0x17, 0x1f, 0x8b, 0x8f, 0x2f, 0xc7} }
NS_DEFINE_CID(kCoreServiceA_CID, CORE_SERVICE_A_CID);


#define EXT_SERVICE_A_CID             \
        { 0xfe64efb7, 0xc5ab, 0x41a6, \
        { 0xb6, 0x39, 0xe6, 0xc0, 0xf4, 0x83, 0x18, 0x1e} }
NS_DEFINE_CID(kExtServiceA_CID, EXT_SERVICE_A_CID);


#define CORE_SERVICE_B_CID             \
        { 0xd04d1298, 0x6dac, 0x459b, \
        { 0xa1, 0x3b, 0xbc, 0xab, 0x23, 0x57, 0x30, 0xa0 } }
NS_DEFINE_CID(kCoreServiceB_CID, CORE_SERVICE_B_CID);


#define EXT_SERVICE_B_CID             \
        { 0xe93dadeb, 0xa6f6, 0x4667, \
        { 0xbb, 0xbc, 0xac, 0x8c, 0x6d, 0x44, 0x0b, 0x20 } }
NS_DEFINE_CID(kExtServiceB_CID, EXT_SERVICE_B_CID);

nsresult execRegOrderTest(const char *aTestName, const char *aContractID,
                      const nsCID &aCoreCID, const nsCID &aExtCID)
{
  
  nsresult rv = NS_ERROR_FAILURE;
  nsCOMPtr<nsISupports> coreService = do_CreateInstance(aCoreCID, &rv);
#ifdef DEBUG_brade
  if (rv) fprintf(stderr, "rv: %d (%x)\n", rv, rv);
  fprintf(stderr, "coreService: %p\n", coreService.get());
#endif
  if (NS_FAILED(rv))
  {
    fail("%s FAILED - cannot create core service\n", aTestName);
    return rv;
  }

  
  nsCOMPtr<nsISupports> extService = do_CreateInstance(aExtCID, &rv);
#ifdef DEBUG_brade
  if (rv) fprintf(stderr, "rv: %d (%x)\n", rv, rv);
  fprintf(stderr, "extService: %p\n", extService.get());
#endif
  if (NS_FAILED(rv))
  {
    fail("%s FAILED - cannot create extension service\n", aTestName);
    return rv;
  }

  




  nsCOMPtr<nsISupports> service = do_CreateInstance(aContractID, &rv);
#ifdef DEBUG_brade
  if (rv) fprintf(stderr, "rv: %d (%x)\n", rv, rv);
  fprintf(stderr, "service: %p\n", service.get());
#endif
  if (NS_FAILED(rv))
  {
    fail("%s FAILED - cannot create service\n", aTestName);
    return rv;
  }

  if (service != extService)
  {
    fail("%s FAILED - wrong service registered\n", aTestName);
    return NS_ERROR_FAILURE;
  }

  passed(aTestName);
  return NS_OK;
}

nsresult TestRegular()
{
  return execRegOrderTest("TestRegular", SERVICE_A_CONTRACT_ID,
                          kCoreServiceA_CID, kExtServiceA_CID);
}

nsresult TestJar()
{
  return execRegOrderTest("TestJar", SERVICE_B_CONTRACT_ID,
                          kCoreServiceB_CID, kExtServiceB_CID);
}

bool TestContractFirst()
{
  nsCOMPtr<nsIComponentRegistrar> r;
  NS_GetComponentRegistrar(getter_AddRefs(r));

  nsCID* cid = nullptr;
  nsresult rv = r->ContractIDToCID("@mozilla.org/RegTestOrderC;1", &cid);
  if (NS_FAILED(rv)) {
    fail("RegTestOrderC: contract not registered");
    return false;
  }

  nsCID goodcid;
  goodcid.Parse("{ada15884-bb89-473c-8b50-dcfbb8447ff4}");

  if (!goodcid.Equals(*cid)) {
    fail("RegTestOrderC: CID doesn't match");
    return false;
  }

  passed("RegTestOrderC");
  return true;
}

static already_AddRefed<nsIFile>
GetRegDirectory(const char* basename, const char* dirname, const char* leafname)
{
    nsCOMPtr<nsIFile> f;
    nsresult rv = NS_NewNativeLocalFile(nsDependentCString(basename), true,
                                        getter_AddRefs(f));
    if (NS_FAILED(rv))
        return nullptr;

    f->AppendNative(nsDependentCString(dirname));
    if (leafname)
        f->AppendNative(nsDependentCString(leafname));
    return f.forget();
}



int main(int argc, char** argv)
{
  if (argc < 2)
  {
    fprintf(stderr, "not enough arguments -- need registration dir path\n");
    return 1;
  }

  ScopedLogging logging;

#ifdef XP_WIN
  
  size_t regPathLen = strlen(argv[1]);
  char* regPath = new char[regPathLen + 1];
  for (size_t i = 0; i < regPathLen; i++) {
    char curr = argv[1][i];
    regPath[i] = (curr == '/') ? '\\' : curr;
  }
  regPath[regPathLen] = '\0';
#else
  const char *regPath = argv[1];
#endif

  XRE_AddManifestLocation(NS_EXTENSION_LOCATION,
                          nsCOMPtr<nsIFile>(GetRegDirectory(regPath, "core", "component.manifest")));
  XRE_AddManifestLocation(NS_EXTENSION_LOCATION,
                          nsCOMPtr<nsIFile>(GetRegDirectory(regPath, "extension", "extComponent.manifest")));
  XRE_AddJarManifestLocation(NS_EXTENSION_LOCATION,
                          nsCOMPtr<nsIFile>(GetRegDirectory(regPath, "extension2.jar", nullptr)));
  ScopedXPCOM xpcom("RegistrationOrder");
  if (xpcom.failed())
    return 1;

  int rv = 0;
  if (NS_FAILED(TestRegular()))
    rv = 1;

  if (NS_FAILED(TestJar()))
    rv = 1;

  if (!TestContractFirst())
    rv = 1;

  return rv;
}
