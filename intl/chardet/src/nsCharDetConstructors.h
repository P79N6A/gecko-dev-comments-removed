










































#ifndef nsCharDetConstructors_h__
#define nsCharDetConstructors_h__


#include "nsISupports.h"
#include "nsICharsetDetector.h"
#include "nsICharsetAlias.h"
#include "nsDocumentCharsetInfo.h"
#include "nsICharsetDetectionAdaptor.h"
#include "nsICharsetDetectionObserver.h"
#include "nsIStringCharsetDetector.h"
#include "nsCyrillicDetector.h"
#include "nsDocumentCharsetInfoCID.h"
#include "nsXMLEncodingCID.h"
#include "nsCharsetDetectionAdaptorCID.h"

NS_GENERIC_FACTORY_CONSTRUCTOR(nsDocumentCharsetInfo)
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
