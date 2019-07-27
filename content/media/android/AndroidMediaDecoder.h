




#if !defined(MediaPluginDecoder_h_)
#define MediaPluginDecoder_h_

#include "MediaDecoder.h"
#include "MediaPluginDecoder.h"

namespace mozilla {

class MediaPluginDecoder : public MediaDecoder
{
  nsCString mType;
public:
  MediaPluginDecoder(const nsACString& aType);

  const nsresult GetContentType(nsACString& aType) const {
    aType = mType;
    return NS_OK;
  }

  virtual MediaDecoder* Clone() { return new MediaPluginDecoder(mType); }
  virtual MediaDecoderStateMachine* CreateStateMachine();
};

} 

#endif
