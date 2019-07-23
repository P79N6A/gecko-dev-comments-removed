





































#ifndef __nsPluginDirServiceProvider_h__
#define __nsPluginDirServiceProvider_h__

#include "nsIDirectoryService.h"

class nsISimpleEnumerator;



#define NS_WIN_JRE_SCAN_KEY            "plugin.scan.SunJRE"
#define NS_WIN_ACROBAT_SCAN_KEY        "plugin.scan.Acrobat"
#define NS_WIN_QUICKTIME_SCAN_KEY      "plugin.scan.Quicktime"
#define NS_WIN_WMP_SCAN_KEY            "plugin.scan.WindowsMediaPlayer"
#define NS_WIN_4DOTX_SCAN_KEY          "plugin.scan.4xPluginFolder"





class nsPluginDirServiceProvider : public nsIDirectoryServiceProvider
{
public:
   nsPluginDirServiceProvider();
   
   NS_DECL_ISUPPORTS
   NS_DECL_NSIDIRECTORYSERVICEPROVIDER

#ifdef XP_WIN
   static nsresult GetPLIDDirectories(nsISimpleEnumerator **aEnumerator);
#endif

protected:
   virtual ~nsPluginDirServiceProvider();
};

#endif 
