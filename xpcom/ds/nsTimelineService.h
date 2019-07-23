




































#include "nsITimelineService.h"

#ifdef MOZ_TIMELINE

#define NS_TIMELINESERVICE_CID \
{ /* a335edf0-3daf-11d5-b67d-000064657374 */ \
    0xa335edf0, \
        0x3daf, \
        0x11d5, \
        {0xb6, 0x7d, 0x00, 0x00, 0x64, 0x65, 0x73, 0x74}}

#define NS_TIMELINESERVICE_CONTRACTID "@mozilla.org;timeline-service;1"
#define NS_TIMELINESERVICE_CLASSNAME "Timeline Service"

class nsTimelineService : public nsITimelineService
{
public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSITIMELINESERVICE

  nsTimelineService();

private:
  ~nsTimelineService() {}
};

#endif
