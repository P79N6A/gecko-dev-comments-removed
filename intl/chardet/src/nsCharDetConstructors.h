










































#ifndef nsCharDetConstructors_h__
#define nsCharDetConstructors_h__


#include "nsISupports.h"
#include "nsMetaCharsetCID.h"
#include "nsICharsetDetector.h"
#include "nsICharsetAlias.h"
#include "nsMetaCharsetObserver.h"
#include "nsDocumentCharsetInfo.h"
#include "nsXMLEncodingObserver.h"
#include "nsICharsetDetectionAdaptor.h"
#include "nsICharsetDetectionObserver.h"
#include "nsDetectionAdaptor.h"
#include "nsIStringCharsetDetector.h"
#include "nsPSMDetectors.h"
#include "nsCyrillicDetector.h"
#include "nsDocumentCharsetInfoCID.h"
#include "nsXMLEncodingCID.h"
#include "nsCharsetDetectionAdaptorCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsMetaCharsetObserver)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDocumentCharsetInfo)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsXMLEncodingObserver)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsDetectionAdaptor)
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
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRUProbDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUKProbDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsRUStringProbDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsUKStringProbDetector)

#ifdef INCLUDE_DBGDETECTOR
NS_GENERIC_FACTORY_CONSTRUCTOR(ns1stBlkDbgDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(ns2ndBlkDbgDetector)
NS_GENERIC_FACTORY_CONSTRUCTOR(nsLastBlkDbgDetector)
#endif 


#endif
