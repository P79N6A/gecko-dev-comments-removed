





































#ifndef nsProfileCollector_h_
#define nsProfileCollector_h_

#include "nsIMetricsCollector.h"

class nsISupports;
class nsIMetricsEventItem;
class nsIPropertyBag;
#ifndef MOZ_PLACES_BOOKMARKS
class nsIRDFResource;
#endif





class nsProfileCollector : public nsIMetricsCollector
{
 public:
  NS_DECL_ISUPPORTS
  NS_DECL_NSIMETRICSCOLLECTOR

  nsProfileCollector();

 private:
  class PluginEnumerator;
  class ExtensionEnumerator;
  class BookmarkCounter;

  ~nsProfileCollector();

  
  nsresult LogCPU(nsIMetricsEventItem *profile);
  nsresult LogMemory(nsIMetricsEventItem *profile);
  nsresult LogOS(nsIMetricsEventItem *profile);
  nsresult LogInstall(nsIMetricsEventItem *profile);
  nsresult LogExtensions(nsIMetricsEventItem *profile);
  nsresult LogPlugins(nsIMetricsEventItem *profile);
  nsresult LogDisplay(nsIMetricsEventItem *profile);
  nsresult LogBookmarks(nsIMetricsEventItem *profile);

#ifdef MOZ_PLACES_BOOKMARKS
  void LogBookmarkLocation(nsIMetricsEventItem *bookmarksItem,
                           const nsACString &location,
                           BookmarkCounter *counter,
                           PRInt64 root, PRBool deep);
#else
  void LogBookmarkLocation(nsIMetricsEventItem *bookmarksItem,
                           const nsACString &location,
                           BookmarkCounter *counter,
                           nsIRDFResource *root, PRBool deep);
#endif

  
  PRBool mLoggedProfile;
};

#define NS_PROFILECOLLECTOR_CLASSNAME "Profile Collector"
#define NS_PROFILECOLLECTOR_CID \
{ 0x9d5d472d, 0x88c7, 0x4cb2, {0xa6, 0xfb, 0x1f, 0x8e, 0x4d, 0xb5, 0x7e, 0x7e}}

#endif  
