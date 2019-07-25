




































#include "mozilla/ModuleUtils.h"
#include "nsCOMPtr.h"
#include "nsICategoryManager.h"
#include "nsIServiceManager.h"

#include "nsCharDetConstructors.h"

NS_DEFINE_NAMED_CID(NS_META_CHARSET_CID);
NS_DEFINE_NAMED_CID(NS_DOCUMENTCHARSETINFO_CID);
NS_DEFINE_NAMED_CID(NS_XML_ENCODING_CID);
NS_DEFINE_NAMED_CID(NS_CHARSET_DETECTION_ADAPTOR_CID);
NS_DEFINE_NAMED_CID(NS_RU_PROBDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_UK_PROBDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_RU_STRING_PROBDETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_UK_STRING_PROBDETECTOR_CID);
#ifdef INCLUDE_DBGDETECTOR
NS_DEFINE_NAMED_CID(NS_1STBLKDBG_DETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_2NDBLKDBG_DETECTOR_CID);
NS_DEFINE_NAMED_CID(NS_LASTBLKDBG_DETECTOR_CID);
#endif 

static const mozilla::Module::CIDEntry kChardetCIDs[] = {
  { &kNS_META_CHARSET_CID, false, NULL, nsMetaCharsetObserverConstructor },
  { &kNS_DOCUMENTCHARSETINFO_CID, false, NULL, nsDocumentCharsetInfoConstructor },
  { &kNS_XML_ENCODING_CID, false, NULL, nsXMLEncodingObserverConstructor },
  { &kNS_CHARSET_DETECTION_ADAPTOR_CID, false, NULL, nsDetectionAdaptorConstructor },
  { &kNS_RU_PROBDETECTOR_CID, false, NULL, nsRUProbDetectorConstructor },
  { &kNS_UK_PROBDETECTOR_CID, false, NULL, nsUKProbDetectorConstructor },
  { &kNS_RU_STRING_PROBDETECTOR_CID, false, NULL, nsRUStringProbDetectorConstructor },
  { &kNS_UK_STRING_PROBDETECTOR_CID, false, NULL, nsUKStringProbDetectorConstructor },
#ifdef INCLUDE_DBGDETECTOR
  { &kNS_1STBLKDBG_DETECTOR_CID, false, NULL, ns1stBlkDbgDetectorConstructor },
  { &kNS_2NDBLKDBG_DETECTOR_CID, false, NULL, ns2ndBlkDbgDetectorConstructor },
  { &kNS_LASTBLKDBG_DETECTOR_CID, false, NULL, nsLastBlkDbgDetectorConstructor },
#endif 
  { NULL }
};

static const mozilla::Module::ContractIDEntry kChardetContracts[] = {
  { NS_META_CHARSET_CONTRACTID, &kNS_META_CHARSET_CID },
  { NS_DOCUMENTCHARSETINFO_CONTRACTID, &kNS_DOCUMENTCHARSETINFO_CID },
  { NS_XML_ENCODING_CONTRACTID, &kNS_XML_ENCODING_CID },
  { NS_CHARSET_DETECTION_ADAPTOR_CONTRACTID, &kNS_CHARSET_DETECTION_ADAPTOR_CID },
  { NS_CHARSET_DETECTOR_CONTRACTID_BASE "ruprob", &kNS_RU_PROBDETECTOR_CID },
  { NS_CHARSET_DETECTOR_CONTRACTID_BASE "ukprob", &kNS_UK_PROBDETECTOR_CID },
  { NS_STRCDETECTOR_CONTRACTID_BASE "ruprob", &kNS_RU_STRING_PROBDETECTOR_CID },
  { NS_STRCDETECTOR_CONTRACTID_BASE "ukprob", &kNS_UK_STRING_PROBDETECTOR_CID },
#ifdef INCLUDE_DBGDETECTOR
  { NS_CHARSET_DETECTOR_CONTRACTID_BASE "1stblkdbg", &kNS_1STBLKDBG_DETECTOR_CID },
  { NS_CHARSET_DETECTOR_CONTRACTID_BASE "2ndblkdbg", &kNS_2NDBLKDBG_DETECTOR_CID },
  { NS_CHARSET_DETECTOR_CONTRACTID_BASE "lastblkdbg", &kNS_LASTBLKDBG_DETECTOR_CID },
#endif 
  { NULL }
};

static const mozilla::Module::CategoryEntry kChardetCategories[] = {
  { "parser-service-category", "Meta Charset Service", NS_META_CHARSET_CONTRACTID },
  { NS_CHARSET_DETECTOR_CATEGORY, "off", "off" },
  { NS_CHARSET_DETECTOR_CATEGORY, "ruprob", NS_CHARSET_DETECTOR_CONTRACTID_BASE "ruprob" },
  { NS_CHARSET_DETECTOR_CATEGORY, "ukprob", NS_CHARSET_DETECTOR_CONTRACTID_BASE "ukprob" },
  { NULL }
};

static const mozilla::Module kChardetModule = {
  mozilla::Module::kVersion,
  kChardetCIDs,
  kChardetContracts,
  kChardetCategories
};

NSMODULE_DEFN(nsChardetModule) = &kChardetModule;
