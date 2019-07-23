




































#include "nsIGenericFactory.h"
#include "nsCOMPtr.h"
#include "nsICategoryManager.h"
#include "nsIServiceManager.h"

#include "nsCharDetConstructors.h"


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

static NS_METHOD
DeleteCategoryEntry(const char* category,
                    const char* key)
{
  nsresult rv;
  nsCOMPtr<nsICategoryManager> 
    categoryManager(do_GetService("@mozilla.org/categorymanager;1", &rv));
  if (NS_FAILED(rv)) return rv;
  
  return categoryManager->DeleteCategoryEntry(category, key, PR_TRUE);
}

static NS_METHOD
nsMetaCharsetObserverRegistrationProc(nsIComponentManager *aCompMgr,
                                      nsIFile *aPath,
                                      const char *registryLocation,
                                      const char *componentType,
                                      const nsModuleComponentInfo *info)
{
  return AddCategoryEntry("parser-service-category", 
                          "Meta Charset Service",
                          NS_META_CHARSET_CONTRACTID);
}

static NS_METHOD
nsMetaCharsetObserverUnegistrationProc(nsIComponentManager *aCompMgr,
                                       nsIFile *aPath,
                                       const char *registryLocation,
                                       const nsModuleComponentInfo *info)
{
  return DeleteCategoryEntry("parser-service-category",
                             "Meta Charset Service");
}

static NS_METHOD
nsDetectionAdaptorRegistrationProc(nsIComponentManager *aCompMgr,
                                   nsIFile *aPath,
                                   const char *registryLocation,
                                   const char *componentType,
                                   const nsModuleComponentInfo *info)
{
  return AddCategoryEntry(NS_CHARSET_DETECTOR_CATEGORY, "off", "off");
}

static NS_METHOD
nsRUProbDetectorRegistrationProc(nsIComponentManager *aCompMgr,
                                 nsIFile *aPath,
                                 const char *registryLocation,
                                 const char *componentType,
                                 const nsModuleComponentInfo *info)
{
  return AddCategoryEntry(NS_CHARSET_DETECTOR_CATEGORY,
                          "ruprob",
                          info->mContractID);
}

static NS_METHOD
nsUKProbDetectorRegistrationProc(nsIComponentManager *aCompMgr,
                                 nsIFile *aPath,
                                 const char *registryLocation,
                                 const char *componentType,
                                 const nsModuleComponentInfo *info)
{
  return AddCategoryEntry(NS_CHARSET_DETECTOR_CATEGORY,
                          "ukprob",
                          info->mContractID);
}


static const nsModuleComponentInfo components[] =
{
 { "Meta Charset", NS_META_CHARSET_CID, 
    NS_META_CHARSET_CONTRACTID, nsMetaCharsetObserverConstructor, 
    nsMetaCharsetObserverRegistrationProc, nsMetaCharsetObserverUnegistrationProc,
    NULL},
 { "Document Charset Info", NS_DOCUMENTCHARSETINFO_CID, 
    NS_DOCUMENTCHARSETINFO_CONTRACTID, nsDocumentCharsetInfoConstructor, 
    NULL, NULL},
 { "XML Encoding", NS_XML_ENCODING_CID, 
    NS_XML_ENCODING_CONTRACTID, nsXMLEncodingObserverConstructor, 
    NULL, NULL},
 { "Charset Detection Adaptor", NS_CHARSET_DETECTION_ADAPTOR_CID, 
    NS_CHARSET_DETECTION_ADAPTOR_CONTRACTID, nsDetectionAdaptorConstructor, 
    nsDetectionAdaptorRegistrationProc, NULL},
 { "Probability based Russian Charset Detector", NS_RU_PROBDETECTOR_CID, 
    NS_CHARSET_DETECTOR_CONTRACTID_BASE "ruprob", nsRUProbDetectorConstructor, 
    nsRUProbDetectorRegistrationProc, NULL},
 { "Probability based Ukrainian Charset Detector", NS_UK_PROBDETECTOR_CID, 
    NS_CHARSET_DETECTOR_CONTRACTID_BASE "ukprob", nsUKProbDetectorConstructor, 
    nsUKProbDetectorRegistrationProc, NULL},
 { "Probability based Russian String Charset Detector", NS_RU_STRING_PROBDETECTOR_CID, 
    NS_STRCDETECTOR_CONTRACTID_BASE "ruprob", nsRUStringProbDetectorConstructor, 
    NULL, NULL},
 { "Probability based Ukrainian String Charset Detector", NS_UK_STRING_PROBDETECTOR_CID, 
    NS_STRCDETECTOR_CONTRACTID_BASE "ukprob", nsUKStringProbDetectorConstructor, 
   NULL, NULL},
#ifdef INCLUDE_DBGDETECTOR
 { "Debugging Detector 1st block", NS_1STBLKDBG_DETECTOR_CID, 
    NS_CHARSET_DETECTOR_CONTRACTID_BASE "1stblkdbg", ns1stBlkDbgDetectorConstructor, 
    NULL, NULL},
 { "Debugging Detector 2nd block", NS_2NDBLKDBG_DETECTOR_CID, 
    NS_CHARSET_DETECTOR_CONTRACTID_BASE "2ndblkdbg", ns2ndBlkDbgDetectorConstructor, 
    NULL, NULL},
 { "Debugging Detector Last block", NS_LASTBLKDBG_DETECTOR_CID, 
    NS_CHARSET_DETECTOR_CONTRACTID_BASE "lastblkdbg", nsLastBlkDbgDetectorConstructor, 
    NULL, NULL},
#endif 
};


NS_IMPL_NSGETMODULE(nsChardetModule, components)
