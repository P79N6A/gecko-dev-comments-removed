




#if !defined(AndroidMediaDecoder_h_)
#define AndroidMediaDecoder_h_

#include "MediaDecoder.h"
#include "AndroidMediaDecoder.h"

namespace mozilla {

class AndroidMediaDecoder : public MediaDecoder
{
  nsCString mType;
public:
  AndroidMediaDecoder(const nsACString& aType);

  const nsresult GetContentType(nsACString& aType) const {
    aType = mType;
    return NS_OK;
  }

  virtual MediaDecoder* Clone() { return new AndroidMediaDecoder(mType); }
  virtual MediaDecoderStateMachine* CreateStateMachine();
};

} 

#endif
