



#ifndef _WEBRTC_GLOBAL_INFORMATION_H_
#define _WEBRTC_GLOBAL_INFORMATION_H_

#include "nsString.h"
#include "mozilla/dom/BindingDeclarations.h" 

namespace mozilla {
class PeerConnectionImpl;
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

  static void SetDebugLevel(const GlobalObject& aGlobal, int32_t aLevel);
  static int32_t DebugLevel(const GlobalObject& aGlobal);

  static void SetAecDebug(const GlobalObject& aGlobal, bool aEnable);
  static bool AecDebug(const GlobalObject& aGlobal);

  static void StoreLongTermICEStatistics(PeerConnectionImpl& aPc);

private:
  WebrtcGlobalInformation() = delete;
  WebrtcGlobalInformation(const WebrtcGlobalInformation& aOrig) = delete;
  WebrtcGlobalInformation& operator=(
    const WebrtcGlobalInformation& aRhs) = delete;
};

} 
} 

#endif  
