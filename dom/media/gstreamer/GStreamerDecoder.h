





#if !defined(GStreamerDecoder_h_)
#define GStreamerDecoder_h_

#include "MediaDecoder.h"
#include "nsXPCOMStrings.h"

namespace mozilla {

class GStreamerDecoder : public MediaDecoder
{
public:
  virtual MediaDecoder* Clone() { return new GStreamerDecoder(); }
  virtual MediaDecoderStateMachine* CreateStateMachine();
  static bool CanHandleMediaType(const nsACString& aMIMEType, const nsAString* aCodecs);
};

} 

#endif
