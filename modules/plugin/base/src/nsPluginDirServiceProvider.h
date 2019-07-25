





































#ifndef nsPluginDirServiceProvider_h_
#define nsPluginDirServiceProvider_h_

#include "nsIDirectoryService.h"

#if defined (XP_WIN)
#include "nsCOMArray.h"
#endif

class nsISimpleEnumerator;



#define NS_WIN_JRE_SCAN_KEY            "plugin.scan.SunJRE"
#define NS_WIN_ACROBAT_SCAN_KEY        "plugin.scan.Acrobat"
#define NS_WIN_QUICKTIME_SCAN_KEY      "plugin.scan.Quicktime"
#define NS_WIN_WMP_SCAN_KEY            "plugin.scan.WindowsMediaPlayer"





class nsPluginDirServiceProvider : public nsIDirectoryServiceProvider
{
public:
   nsPluginDirServiceProvider();
   
   NS_DECL_ISUPPORTS
   NS_DECL_NSIDIRECTORYSERVICEPROVIDER

#ifdef XP_WIN
   static nsresult GetPLIDDirectories(nsISimpleEnumerator **aEnumerator);
private:
   static nsresult GetPLIDDirectoriesWithRootKey(PRUint32 aKey,
     nsCOMArray<nsILocalFile> &aDirs);
#endif

protected:
   virtual ~nsPluginDirServiceProvider();
};

#endif 
