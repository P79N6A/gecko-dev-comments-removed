





#if !defined(GStreamerFormatHelper_h_)
#define GStreamerFormatHelper_h_

#include <gst/gst.h>
#include <mozilla/Types.h>
#include "nsXPCOMStrings.h"

class GStreamerFormatHelper {
  




  public:
    static GStreamerFormatHelper* Instance();
    ~GStreamerFormatHelper();

    bool CanHandleMediaType(const nsACString& aMIMEType,
                            const nsAString* aCodecs);

   static void Shutdown();

  private:
    GStreamerFormatHelper();
    GstCaps* ConvertFormatsToCaps(const char* aMIMEType,
                                  const nsAString* aCodecs);
    char* const *CodecListFromCaps(GstCaps* aCaps);
    bool HaveElementsToProcessCaps(GstCaps* aCaps);
    GList* GetFactories();

    static GStreamerFormatHelper* gInstance;

    
    static char const *const mContainers[6][2];

    
    static char const *const mCodecs[9][2];

    






    GList* mFactories;

    


    uint32_t mCookie;
};

#endif
