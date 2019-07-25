






#include "nsIModule.h"
#include "nsIFactory.h"

#include "nsIComponentManager.h"
#include "nsIComponentRegistrar.h"
#include "nsIContentSniffer.h"


#define NS_MEDIA_SNIFFER_CID \
{0x3fdd6c28, 0x5b87, 0x4e3e, \
{0x8b, 0x57, 0x8e, 0x83, 0xc2, 0x3c, 0x1a, 0x6d}}

#define NS_MEDIA_SNIFFER_CONTRACTID "@mozilla.org/media/sniffer;1"

class nsMediaSniffer : public nsIContentSniffer
{
  public:
    NS_DECL_ISUPPORTS
    NS_DECL_NSICONTENTSNIFFER
  protected:
    ~nsMediaSniffer() {};

#define PATTERN_ENTRY(mask, pattern, contentType) \
    {(const uint8_t*)mask, (const uint8_t*)pattern, sizeof(mask) - 1, contentType}

  struct nsMediaSnifferEntry {
    const uint8_t* mMask;
    const uint8_t* mPattern;
    const uint32_t mLength;
    const char* mContentType;
  };

  static nsMediaSnifferEntry sSnifferEntries[];
};
