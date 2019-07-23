






































#include "TestHarness.h"
#include "nsILocalFile.h"
#include "nsIDirectoryService.h"
#include "nsDirectoryServiceDefs.h"
#include "nsCOMArray.h"
#include "nsArrayEnumerator.h"

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


#ifdef DEBUG_brade
inline void
debugPrintPath(const char *aPrefix, nsIFile *aFile)
{
  if (!aPrefix || !aFile)
    return;

  nsCAutoString path;
  nsresult rv = aFile->GetNativePath(path);
  if (NS_FAILED(rv))
    fprintf(stderr, "%s:  GetNativePath failed 0x%x\n", aPrefix, rv);
  else
    fprintf(stderr, "%s: %s\n", aPrefix, path.get());
}
#endif 




class RegOrderDirSvcProvider: public nsIDirectoryServiceProvider2 {
public:
  RegOrderDirSvcProvider(const char *aRegPath)
    : mRegPath(aRegPath)
  {
  }

  NS_DECL_ISUPPORTS
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER
  NS_DECL_NSIDIRECTORYSERVICEPROVIDER2

private:
  nsresult getRegDirectory(const char *aDirName, nsIFile **_retval);
  const char *mRegPath;
};

NS_IMPL_ISUPPORTS2(RegOrderDirSvcProvider,
                   nsIDirectoryServiceProvider,
                   nsIDirectoryServiceProvider2)

NS_IMETHODIMP
RegOrderDirSvcProvider::GetFiles(const char *aKey, nsISimpleEnumerator **aEnum)
{
  nsresult rv = NS_ERROR_FAILURE;
  *aEnum = nsnull;

#ifdef DEBUG_brade
    fprintf(stderr, "GetFiles(%s)\n", aKey);
#endif

  if (0 == strcmp(aKey, NS_XPCOM_COMPONENT_DIR_LIST))
  {
    nsCOMPtr<nsIFile> coreDir;
    rv = getRegDirectory("core", getter_AddRefs(coreDir));
    if (NS_SUCCEEDED(rv))
    {
      nsCOMPtr<nsIFile> extDir;
      rv = getRegDirectory("extension", getter_AddRefs(extDir));
      if (NS_SUCCEEDED(rv))
      {
        nsCOMArray<nsIFile> dirArray;
        dirArray.AppendObject(coreDir);
        dirArray.AppendObject(extDir);
        rv = NS_NewArrayEnumerator(aEnum, dirArray);
      }
    }
  }

#ifdef DEBUG_brade
  if (rv) fprintf(stderr, "rv: %d (%x)\n", rv, rv);
#endif
  return rv;
}

NS_IMETHODIMP
RegOrderDirSvcProvider::GetFile(const char *aProp, PRBool *aPersistent,
                                nsIFile **_retval)
{
  *_retval = nsnull;
  return NS_ERROR_FAILURE;
}

nsresult
RegOrderDirSvcProvider::getRegDirectory(const char *aDirName, nsIFile **_retval)
{
  nsCOMPtr<nsILocalFile> compDir;
  nsresult rv = NS_NewLocalFile(EmptyString(), PR_TRUE,
                                getter_AddRefs(compDir));
  if (NS_FAILED(rv))
    return rv;

  rv = compDir->InitWithNativePath(nsDependentCString(mRegPath));
  if (NS_FAILED(rv))
    return rv;

  rv = compDir->AppendRelativeNativePath(nsDependentCString(aDirName));
  if (NS_FAILED(rv))
    return rv;

  *_retval = compDir;
  NS_ADDREF(*_retval);
  return NS_OK;
}

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

nsresult TestDeferred()
{
  return execRegOrderTest("TestDeferred", SERVICE_B_CONTRACT_ID,
                          kCoreServiceB_CID, kExtServiceB_CID);
}

int main(int argc, char** argv)
{
  if (argc < 2)
  {
    fprintf(stderr, "not enough arguments -- need registration dir path\n");
    return 1;
  }

  const char *regPath = argv[1];
  RegOrderDirSvcProvider *dirSvcProvider = new RegOrderDirSvcProvider(regPath);
  if (NULL == dirSvcProvider)
  {
    fprintf(stderr, "could not create dirSvcProvider\n");
    return 1;
  }
  

  ScopedXPCOM xpcom("RegistrationOrder", dirSvcProvider);
  if (xpcom.failed())
    return 1;

  int rv = 0;
  if (NS_FAILED(TestRegular()))
    rv = 1;
  if (NS_FAILED(TestDeferred()))
    rv = 1;

  return rv;
}
