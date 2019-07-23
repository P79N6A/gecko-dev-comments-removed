





































#ifndef QECKOGLOBALS_H
#define QECKOGLOBALS_H

#include "prenv.h"

class nsModuleComponentInfo;
class nsIDirectoryServiceProvider;
class nsModuleComponentInfo;
class nsIAppShell;
class nsVoidArray;
class nsIDirectoryServiceProvider;
class nsIWebBrowserChrome;
class QGeckoEmbed;
class nsILocalFile;
class nsISupports;
class QTEmbedDirectoryProvider;

class QGeckoGlobals
{
    friend class QGeckoEmbed;
    friend class QTEmbedDirectoryProvider;
public:
    static void initializeGlobalObjects();
    static void pushStartup();
    static void popStartup();
    static void setPath(const char *aPath);
    static void setCompPath(const char *aPath);
    static void setAppComponents(const nsModuleComponentInfo *aComps,
                                 int aNumComponents);
    static void setProfilePath(const char *aDir, const char *aName);
    static void setDirectoryServiceProvider(nsIDirectoryServiceProvider
                                            *appFileLocProvider);
    static int registerAppComponents();

    static void addEngine(QGeckoEmbed *embed);
    static void removeEngine(QGeckoEmbed *embed);
    static QGeckoEmbed *findPrivateForBrowser(nsIWebBrowserChrome *aBrowser);
    static nsIDirectoryServiceProvider *sAppFileLocProvider;

private:
    static PRUint32                sWidgetCount;
    
    static char                   *sPath;
    
    static char                   *sCompPath;
    
    static const nsModuleComponentInfo  *sAppComps;
    static int                     sNumAppComps;
    
    static nsIAppShell            *sAppShell;
    
    static nsILocalFile           *sProfileDir;
    static nsISupports            *sProfileLock;


    
    static nsVoidArray            *sWindowList;
};

#endif
