



































let EXPORTED_SYMBOLS = ["Services"];

const Ci = Components.interfaces;
const Cc = Components.classes;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

let Services = {};

XPCOMUtils.defineLazyGetter(Services, "prefs", function () {
  return Cc["@mozilla.org/preferences-service;1"]
           .getService(Ci.nsIPrefService)
           .QueryInterface(Ci.nsIPrefBranch2);
});

XPCOMUtils.defineLazyGetter(Services, "appinfo", function () {
  return Cc["@mozilla.org/xre/app-info;1"]
           .getService(Ci.nsIXULAppInfo)
           .QueryInterface(Ci.nsIXULRuntime);
});

XPCOMUtils.defineLazyGetter(Services, "dirsvc", function () {
  return Cc["@mozilla.org/file/directory_service;1"]
           .getService(Ci.nsIDirectoryService)
           .QueryInterface(Ci.nsIProperties);
});

XPCOMUtils.defineLazyServiceGetter(Services, "contentPrefs",
                                   "@mozilla.org/content-pref/service;1",
                                   "nsIContentPrefService");

XPCOMUtils.defineLazyServiceGetter(Services, "wm",
                                   "@mozilla.org/appshell/window-mediator;1",
                                   "nsIWindowMediator");

XPCOMUtils.defineLazyServiceGetter(Services, "obs",
                                   "@mozilla.org/observer-service;1",
                                   "nsIObserverService");

XPCOMUtils.defineLazyServiceGetter(Services, "perms",
                                   "@mozilla.org/permissionmanager;1",
                                   "nsIPermissionManager");

XPCOMUtils.defineLazyServiceGetter(Services, "io",
                                   "@mozilla.org/network/io-service;1",
                                   "nsIIOService2");

XPCOMUtils.defineLazyServiceGetter(Services, "prompt",
                                   "@mozilla.org/embedcomp/prompt-service;1",
                                   "nsIPromptService");

#ifdef MOZ_TOOLKIT_SEARCH
XPCOMUtils.defineLazyServiceGetter(Services, "search",
                                   "@mozilla.org/browser/search-service;1",
                                   "nsIBrowserSearchService");
#endif

XPCOMUtils.defineLazyServiceGetter(Services, "storage",
                                   "@mozilla.org/storage/service;1",
                                   "mozIStorageService");

XPCOMUtils.defineLazyServiceGetter(Services, "vc",
                                   "@mozilla.org/xpcom/version-comparator;1",
                                   "nsIVersionComparator");

XPCOMUtils.defineLazyServiceGetter(Services, "locale",
                                   "@mozilla.org/intl/nslocaleservice;1",
                                   "nsILocaleService");

XPCOMUtils.defineLazyServiceGetter(Services, "scriptloader",
                                   "@mozilla.org/moz/jssubscript-loader;1",
                                   "mozIJSSubScriptLoader");

XPCOMUtils.defineLazyServiceGetter(Services, "ww",
                                   "@mozilla.org/embedcomp/window-watcher;1",
                                   "nsIWindowWatcher");

XPCOMUtils.defineLazyServiceGetter(Services, "tm",
                                   "@mozilla.org/thread-manager;1",
                                   "nsIThreadManager");

XPCOMUtils.defineLazyServiceGetter(Services, "droppedLinkHandler",
                                   "@mozilla.org/content/dropped-link-handler;1",
                                   "nsIDroppedLinkHandler");

XPCOMUtils.defineLazyServiceGetter(Services, "console",
                                   "@mozilla.org/consoleservice;1",
                                   "nsIConsoleService");

XPCOMUtils.defineLazyServiceGetter(Services, "strings",
                                   "@mozilla.org/intl/stringbundle;1",
                                   "nsIStringBundleService");

XPCOMUtils.defineLazyServiceGetter(Services, "urlFormatter",
                                   "@mozilla.org/toolkit/URLFormatterService;1",
                                   "nsIURLFormatter");

XPCOMUtils.defineLazyServiceGetter(Services, "eTLD",
                                   "@mozilla.org/network/effective-tld-service;1",
                                   "nsIEffectiveTLDService");

XPCOMUtils.defineLazyServiceGetter(Services, "cookies",
                                   "@mozilla.org/cookiemanager;1",
                                   "nsICookieManager2");

XPCOMUtils.defineLazyServiceGetter(Services, "logins",
                                   "@mozilla.org/login-manager;1",
                                   "nsILoginManager");
