




#if !defined(nsMediaPluginDecoder_h_)
#define nsMediaPluginDecoder_h_

#include "nsBuiltinDecoder.h"
#include "nsMediaPluginDecoder.h"

namespace mozilla {

class nsMediaPluginDecoder : public nsBuiltinDecoder
{
  nsCString mType;
public:
  nsMediaPluginDecoder(const nsACString& aType);

  const nsresult GetContentType(nsACString& aType) const {
    aType = mType;
    return NS_OK;
  }

  virtual nsBuiltinDecoder* Clone() { return new nsMediaPluginDecoder(mType); }
  virtual nsBuiltinDecoderStateMachine* CreateStateMachine();
};

} 

#endif
