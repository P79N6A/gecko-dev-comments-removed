





#if !defined(GStreamerFormatHelper_h_)
#define GStreamerFormatHelper_h_

#include <gst/gst.h>
#include <mozilla/Types.h>
#include "nsXPCOMStrings.h"

namespace mozilla {

class GStreamerFormatHelper {
  




  public:
    static GStreamerFormatHelper* Instance();
    ~GStreamerFormatHelper();

    bool CanHandleMediaType(const nsACString& aMIMEType,
                            const nsAString* aCodecs);

    bool CanHandleContainerCaps(GstCaps* aCaps);
    bool CanHandleCodecCaps(GstCaps* aCaps);

    static bool IsBlacklistEnabled();
    static bool IsPluginFeatureBlacklisted(GstPluginFeature *aFeature);

    static GstCaps* ConvertFormatsToCaps(const char* aMIMEType,
                                         const nsAString* aCodecs);

    static void Shutdown();

  private:
    GStreamerFormatHelper();
    char* const *CodecListFromCaps(GstCaps* aCaps);
    bool HaveElementsToProcessCaps(GstCaps* aCaps);
    GList* GetFactories();

    static GStreamerFormatHelper* gInstance;

    
    static char const *const mContainers[6][2];

    
    static char const *const mCodecs[9][2];

    



    static bool sLoadOK;

    
    GstCaps* mSupportedContainerCaps;
    GstCaps* mSupportedCodecCaps;

    






    GList* mFactories;

    


    uint32_t mCookie;
};

} 

#endif
