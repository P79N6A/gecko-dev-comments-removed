





































#include "nsICharsetAlias.h"

#include "pratom.h"
#include "nsClassicCharDetDll.h"
#include "nsICharsetDetectionObserver.h"
#include "nsISupports.h"
#include "nsIComponentManager.h"
#include "nsIFactory.h"
#include "nsIServiceManager.h"
#include "nsIGenericFactory.h"
#include "nsCOMPtr.h"
#include "nsICharsetDetector.h"
#include "nsIStringCharsetDetector.h"
#include "nsClassicDetectors.h"


NS_GENERIC_FACTORY_CONSTRUCTOR(nsJACharsetClassicDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsJAStringCharsetClassicDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsKOCharsetClassicDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsKOStringCharsetClassicDetector)

static const nsModuleComponentInfo components[] = {
  { "Classic JA Charset Detector", NS_JA_CLASSIC_DETECTOR_CID,
     NS_CHARSET_DETECTOR_CONTRACTID_BASE "jaclassic", nsJACharsetClassicDetectorConstructor},
  { "Classic JA String Charset Detector", NS_JA_CLASSIC_DETECTOR_CID,
     NS_STRCDETECTOR_CONTRACTID_BASE "jaclassic", nsJAStringCharsetClassicDetectorConstructor},
  { "Classic KO Charset Detector", NS_KO_CLASSIC_DETECTOR_CID,
     NS_CHARSET_DETECTOR_CONTRACTID_BASE "koclassic", nsKOCharsetClassicDetectorConstructor},
  { "Classic KO String Charset Detector", NS_KO_CLASSIC_STRING_DETECTOR_CID,
     NS_STRCDETECTOR_CONTRACTID_BASE "koclassic", nsKOStringCharsetClassicDetectorConstructor}
};

NS_IMPL_NSGETMODULE(nsCharDetModuleClassic, components)

