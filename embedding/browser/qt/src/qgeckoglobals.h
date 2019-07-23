





































#ifndef QECKOGLOBALS_H
#define QECKOGLOBALS_H

#include "prenv.h"

class nsModuleComponentInfo;
class nsIDirectoryServiceProvider;
class nsModuleComponentInfo;
class nsIAppShell;
class nsVoidArray;
class nsProfileDirServiceProvider;
class nsIDirectoryServiceProvider;
class nsIWebBrowserChrome;

class QGeckoGlobals
{
    friend class QGeckoEmbed;
public:
    static void initializeGlobalObjects();
    static void pushStartup();
    static void popStartup();
    static void setCompPath(const char *aPath);
    static void setAppComponents(const nsModuleComponentInfo *aComps,
                                 int aNumComponents);
    static void setProfilePath(const char *aDir, const char *aName);
    static void setDirectoryServiceProvider(nsIDirectoryServiceProvider
                                            *appFileLocProvider);
    static int  startupProfile(void);
    static void shutdownProfile(void);

    static int registerAppComponents();

    static void addEngine(QGeckoEmbed *embed);
    static void removeEngine(QGeckoEmbed *embed);
    static QGeckoEmbed *findPrivateForBrowser(nsIWebBrowserChrome *aBrowser);
private:
    static PRUint32                sWidgetCount;
    
    static char                   *sCompPath;
    
    static const nsModuleComponentInfo  *sAppComps;
    static int                     sNumAppComps;
    
    static nsIAppShell            *sAppShell;
    
    static char                   *sProfileDir;
    static char                   *sProfileName;
    
    static nsProfileDirServiceProvider *sProfileDirServiceProvider;
    static nsIDirectoryServiceProvider *sAppFileLocProvider;

    
    static nsVoidArray            *sWindowList;
};

#endif
