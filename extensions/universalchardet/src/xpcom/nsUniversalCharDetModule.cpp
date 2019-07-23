




































#include "nsICharsetAlias.h"
#include "nsCOMPtr.h"
#include "nsIModule.h"

#include "nspr.h"
#include "nsString.h"
#include "pratom.h"
#include "nsUniversalCharDetDll.h"
#include "nsISupports.h"
#include "nsICategoryManager.h"
#include "nsIComponentManager.h"
#include "nsIFactory.h"
#include "nsIServiceManager.h"
#include "nsICharsetDetector.h"
#include "nsIStringCharsetDetector.h"
#include "nsIGenericFactory.h"

#include "nsUniversalDetector.h"
#include "nsUdetXPCOMWrapper.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(nsUniversalXPCOMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUniversalXPCOMStringDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsJAPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsJAStringPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsKOPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsKOStringPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZHTWPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZHTWStringPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZHCNPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZHCNStringPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZHPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsZHStringPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCJKPSMDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsCJKStringPSMDetector)


static NS_METHOD
AddCategoryEntry(const char* category,
                 const char* key,
                 const char* value)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> 
    categoryManager(do_GetService("@mozilla.org/categorymanager;1", &rv));
  if (NS_FAILED(rv)) return rv;
  
  return categoryManager->AddCategoryEntry(category, key, value, 
                                           PR_TRUE, PR_TRUE,
                                           nsnull);
}

static NS_METHOD nsUniversalCharDetectorRegistrationProc(nsIComponentManager *aCompMgr,
                                          nsIFile *aPath,
                                          const char *registryLocation,
                                          const char *componentType,
                                          const nsModuleComponentInfo *info)
{ 
  return AddCategoryEntry(NS_CHARSET_DETECTOR_CATEGORY,
                          "universal_charset_detector",
                          info->mContractID);
}

static NS_METHOD
nsJAPSMDetectorRegistrationProc(nsIComponentManager *aCompMgr,
                                nsIFile *aPath,
                                const char *registryLocation,
                                const char *componentType,
                                const nsModuleComponentInfo *info)
{
  return AddCategoryEntry(NS_CHARSET_DETECTOR_CATEGORY,
                          "ja_parallel_state_machine",
                          info->mContractID);
}

static NS_METHOD
nsKOPSMDetectorRegistrationProc(nsIComponentManager *aCompMgr,
                                nsIFile *aPath,
                                const char *registryLocation,
                                const char *componentType,
                                const nsModuleComponentInfo *info)
{
  return AddCategoryEntry(NS_CHARSET_DETECTOR_CATEGORY,
                          "ko_parallel_state_machine",
                          info->mContractID);
}

static NS_METHOD
nsZHTWPSMDetectorRegistrationProc(nsIComponentManager *aCompMgr,
                                  nsIFile *aPath,
                                  const char *registryLocation,
                                  const char *componentType,
                                  const nsModuleComponentInfo *info)
{
  return AddCategoryEntry(NS_CHARSET_DETECTOR_CATEGORY,
                          "zhtw_parallel_state_machine",
                          info->mContractID);
}

static NS_METHOD
nsZHCNPSMDetectorRegistrationProc(nsIComponentManager *aCompMgr,
                                  nsIFile *aPath,
                                  const char *registryLocation,
                                  const char *componentType,
                                  const nsModuleComponentInfo *info)
{
  return AddCategoryEntry(NS_CHARSET_DETECTOR_CATEGORY,
                          "zhcn_parallel_state_machine",
                          info->mContractID);
}

static NS_METHOD
nsZHPSMDetectorRegistrationProc(nsIComponentManager *aCompMgr,
                                nsIFile *aPath,
                                const char *registryLocation,
                                const char *componentType,
                                const nsModuleComponentInfo *info)
{
  return AddCategoryEntry(NS_CHARSET_DETECTOR_CATEGORY,
                          "zh_parallel_state_machine",
                          info->mContractID);
}

static NS_METHOD
nsCJKPSMDetectorRegistrationProc(nsIComponentManager *aCompMgr,
                                 nsIFile *aPath,
                                 const char *registryLocation,
                                 const char *componentType,
                                 const nsModuleComponentInfo *info)
{
  return AddCategoryEntry(NS_CHARSET_DETECTOR_CATEGORY,
                          "cjk_parallel_state_machine",
                          info->mContractID);
}


static const nsModuleComponentInfo components[] = 
{
   { "Universal Charset Detector", NS_UNIVERSAL_DETECTOR_CID, 
    NS_CHARSET_DETECTOR_CONTRACTID_BASE "universal_charset_detector", nsUniversalXPCOMDetectorConstructor, 
    nsUniversalCharDetectorRegistrationProc, NULL},
   { "Universal String Charset Detector", NS_UNIVERSAL_STRING_DETECTOR_CID, 
    NS_STRCDETECTOR_CONTRACTID_BASE "universal_charset_detector", nsUniversalXPCOMStringDetectorConstructor, 
    NULL, NULL},
 { "PSM based Japanese Charset Detector", NS_JA_PSMDETECTOR_CID, 
    NS_CHARSET_DETECTOR_CONTRACTID_BASE "ja_parallel_state_machine", nsJAPSMDetectorConstructor, 
    nsJAPSMDetectorRegistrationProc, NULL},
 { "PSM based Japanese String Charset Detector", NS_JA_STRING_PSMDETECTOR_CID, 
    NS_STRCDETECTOR_CONTRACTID_BASE "ja_parallel_state_machine", nsJAStringPSMDetectorConstructor, 
    NULL, NULL},
 { "PSM based Korean Charset Detector", NS_KO_PSMDETECTOR_CID, 
    NS_CHARSET_DETECTOR_CONTRACTID_BASE "ko_parallel_state_machine", nsKOPSMDetectorConstructor, 
    nsKOPSMDetectorRegistrationProc, NULL},
 { "PSM based Korean String Charset Detector", NS_KO_STRING_PSMDETECTOR_CID, 
    NS_STRCDETECTOR_CONTRACTID_BASE "ko_parallel_state_machine", nsKOStringPSMDetectorConstructor, 
    NULL, NULL},
 { "PSM based Traditional Chinese Charset Detector", NS_ZHTW_PSMDETECTOR_CID, 
    NS_CHARSET_DETECTOR_CONTRACTID_BASE "zhtw_parallel_state_machine", nsZHTWPSMDetectorConstructor, 
    nsZHTWPSMDetectorRegistrationProc, NULL},
 { "PSM based Traditional Chinese String Charset Detector", NS_ZHTW_STRING_PSMDETECTOR_CID, 
    NS_STRCDETECTOR_CONTRACTID_BASE "zhtw_parallel_state_machine", nsZHTWStringPSMDetectorConstructor, 
    NULL, NULL},
 { "PSM based Simplified Chinese Charset Detector", NS_ZHCN_PSMDETECTOR_CID, 
    NS_CHARSET_DETECTOR_CONTRACTID_BASE "zhcn_parallel_state_machine", nsZHCNPSMDetectorConstructor, 
    nsZHCNPSMDetectorRegistrationProc, NULL},
 { "PSM based Simplified Chinese String Charset Detector", NS_ZHCN_STRING_PSMDETECTOR_CID, 
    NS_STRCDETECTOR_CONTRACTID_BASE "zhcn_parallel_state_machine", nsZHCNStringPSMDetectorConstructor, 
    NULL, NULL},
 { "PSM based Chinese Charset Detector", NS_ZH_PSMDETECTOR_CID, 
    NS_CHARSET_DETECTOR_CONTRACTID_BASE "zh_parallel_state_machine", nsZHPSMDetectorConstructor, 
    nsZHPSMDetectorRegistrationProc, NULL},
 { "PSM based Chinese String Charset Detector", NS_ZH_STRING_PSMDETECTOR_CID, 
    NS_STRCDETECTOR_CONTRACTID_BASE "zh_parallel_state_machine", nsZHStringPSMDetectorConstructor, 
    NULL, NULL},
 { "PSM based CJK Charset Detector", NS_CJK_PSMDETECTOR_CID, 
    NS_CHARSET_DETECTOR_CONTRACTID_BASE "cjk_parallel_state_machine", nsCJKPSMDetectorConstructor, 
    nsCJKPSMDetectorRegistrationProc, NULL},
 { "PSM based CJK String Charset Detector", NS_CJK_STRING_PSMDETECTOR_CID, 
    NS_STRCDETECTOR_CONTRACTID_BASE "cjk_parallel_state_machine", nsCJKStringPSMDetectorConstructor, 
    NULL, NULL},
};

NS_IMPL_NSGETMODULE(nsUniversalCharDetModule, components)
