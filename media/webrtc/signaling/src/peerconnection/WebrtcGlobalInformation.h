



#ifndef _WEBRTC_GLOBAL_INFORMATION_H_
#define _WEBRTC_GLOBAL_INFORMATION_H_

#include "nsString.h"
#include "mozilla/dom/BindingDeclarations.h" 

namespace sipcc {
class PeerConnectionImpl;
}

namespace mozilla {
class ErrorResult;

namespace dom {
class GlobalObject;
class WebrtcGlobalStatisticsCallback;
class WebrtcGlobalLoggingCallback;

class WebrtcGlobalInformation
{
public:
  static void GetAllStats(const GlobalObject& aGlobal,
                          WebrtcGlobalStatisticsCallback& aStatsCallback,
                          const Optional<nsAString>& pcIdFilter,
                          ErrorResult& aRv);

  static void GetLogging(const GlobalObject& aGlobal,
                         const nsAString& aPattern,
                         WebrtcGlobalLoggingCallback& aLoggingCallback,
                         ErrorResult& aRv);

  static void StoreLongTermICEStatistics(sipcc::PeerConnectionImpl& aPc);

private:
  WebrtcGlobalInformation() MOZ_DELETE;
  WebrtcGlobalInformation(const WebrtcGlobalInformation& aOrig) MOZ_DELETE;
  WebrtcGlobalInformation& operator=(
    const WebrtcGlobalInformation& aRhs) MOZ_DELETE;
};

} 
} 

#endif  

