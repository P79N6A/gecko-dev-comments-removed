




#if !defined(WMFDecoder_h_)
#define WMFDecoder_h_

#include "MediaDecoder.h"

namespace mozilla {




class WMFDecoder : public MediaDecoder
{
public:

  virtual MediaDecoder* Clone() {
    if (!IsWMFEnabled()) {
      return nullptr;
    }
    return new WMFDecoder();
  }

  virtual MediaDecoderStateMachine* CreateStateMachine();

  
  
  
  
  
  static bool GetSupportedCodecs(const nsACString& aType,
                                 char const *const ** aCodecList);

  
  
  static nsresult LoadDLLs();
  static void UnloadDLLs();

  
  
  static bool IsEnabled();
};

} 

#endif
