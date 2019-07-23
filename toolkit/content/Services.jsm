



































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

XPCOMUtils.defineLazyServiceGetter(Services, "wm",
                                   "@mozilla.org/appshell/window-mediator;1",
                                   "nsIWindowMediator");

XPCOMUtils.defineLazyServiceGetter(Services, "obs",
                                   "@mozilla.org/observer-service;1",
                                   "nsIObserverService");

XPCOMUtils.defineLazyServiceGetter(Services, "pm",
                                   "@mozilla.org/permissionmanager;1",
                                   "nsIPermissionManager");

XPCOMUtils.defineLazyServiceGetter(Services, "io",
                                   "@mozilla.org/network/io-service;1",
                                   "nsIIOService2");

XPCOMUtils.defineLazyServiceGetter(Services, "prompt",
                                   "@mozilla.org/embedcomp/prompt-service;1",
                                   "nsIPromptService");

XPCOMUtils.defineLazyServiceGetter(Services, "search",
                                   "@mozilla.org/browser/search-service;1",
                                   "nsIBrowserSearchService");
