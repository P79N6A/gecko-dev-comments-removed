




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function openWindow(aParent, aURL, aTarget, aFeatures, aArgs) {
  let argString = null;
  if (aArgs && !(aArgs instanceof Ci.nsISupportsArray)) {
    argString = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
    argString.data = aArgs;
  }

  return Services.ww.openWindow(aParent, aURL, aTarget, aFeatures, argString || aArgs);
}

function resolveURIInternal(aCmdLine, aArgument) {
  let uri = aCmdLine.resolveURI(aArgument);

  if (!(uri instanceof Ci.nsIFileURL))
    return uri;

  try {
    if (uri.file.exists())
      return uri;
  } catch (e) {
    Cu.reportError(e);
  }

  try {
    let urifixup = Cc["@mozilla.org/docshell/urifixup;1"].getService(Ci.nsIURIFixup);
    uri = urifixup.createFixupURI(aArgument, 0);
  } catch (e) {
    Cu.reportError(e);
  }

  return uri;
}









function needHomepageOverride() {
  let savedmstone = null;
  try {
    savedmstone = Services.prefs.getCharPref("browser.startup.homepage_override.mstone");
  } catch (e) {}

  if (savedmstone == "ignore")
    return "none";

#expand    let ourmstone = "__MOZ_APP_VERSION__";

  if (ourmstone != savedmstone) {
    Services.prefs.setCharPref("browser.startup.homepage_override.mstone", ourmstone);

    return (savedmstone ? "new version" : "new profile");
  }

  return "none";
}

function getHomePage() {
  let url = "about:home";
  try {
    url = Services.prefs.getComplexValue("browser.startup.homepage", Ci.nsIPrefLocalizedString).data;
  } catch (e) { }

  return url;
}

function showPanelWhenReady(aWindow, aPage) {
  aWindow.addEventListener("UIReadyDelayed", function(aEvent) {
    aWindow.BrowserUI.showPanel(aPage);
  }, false);
}

function haveSystemLocale() {
  let localeService = Cc["@mozilla.org/intl/nslocaleservice;1"].getService(Ci.nsILocaleService);
  let systemLocale = localeService.getSystemLocale().getCategory("NSILOCALE_CTYPE");
  return isLocaleAvailable(systemLocale);
}

function checkCurrentLocale() {
  if (Services.prefs.prefHasUserValue("general.useragent.locale")) {
    
    var buildID = Cc["@mozilla.org/xre/app-info;1"].getService(Ci.nsIXULAppInfo).platformBuildID;
    let localeBuildID = Services.prefs.getCharPref("extensions.compatability.locales.buildid");
    if (buildID != localeBuildID)
      return false;

    let currentLocale = Services.prefs.getCharPref("general.useragent.locale");
    return isLocaleAvailable(currentLocale);
  }
  return true;
}

function isLocaleAvailable(aLocale) {
  let chrome = Cc["@mozilla.org/chrome/chrome-registry;1"].getService(Ci.nsIXULChromeRegistry);
  chrome.QueryInterface(Ci.nsIToolkitChromeRegistry);
  let availableLocales = chrome.getLocalesForPackage("browser");
 
  let locale = aLocale.split("-")[0];
  let localeRegEx = new RegExp("^" + locale);
 
  while (availableLocales.hasMore()) {
    let locale = availableLocales.getNext();
    if (localeRegEx.test(locale))
      return true;
  }
  return false;
}

function BrowserCLH() { }

BrowserCLH.prototype = {
  
  
  
  handle: function fs_handle(aCmdLine) {
    
    
    
    
    
    
    if (aCmdLine.findFlag("silent", false) > -1) {
      let searchService = Services.search;
      let autoComplete = Cc["@mozilla.org/autocomplete/search;1?name=history"].
                         getService(Ci.nsIAutoCompleteSearch);
      return;
    }

    
    let chromeParam = aCmdLine.handleFlagWithParam("chrome", false);
    if (chromeParam) {
      try {
        
        let features = "chrome,dialog=no,all";
        let uri = resolveURIInternal(aCmdLine, chromeParam);
        let netutil = Cc["@mozilla.org/network/util;1"].getService(Ci.nsINetUtil);
        if (!netutil.URIChainHasFlags(uri, Ci.nsIHttpProtocolHandler.URI_INHERITS_SECURITY_CONTEXT)) {
          openWindow(null, uri.spec, "_blank", features, null);

          
          aCmdLine.preventDefault = true;
        }
      } catch (e) {
        Cu.reportError(e);
      }
      return;
    }

    
    let alertFlag = aCmdLine.handleFlagWithParam("alert", false);

    
    let appFlag = aCmdLine.handleFlagWithParam("webapp", false);
    let appURI;
    if (appFlag)
      appURI = resolveURIInternal(aCmdLine, appFlag);

    
    let uris = [];

    
    let uriFlag = aCmdLine.handleFlagWithParam("url", false);
    if (uriFlag) {
      let uri = resolveURIInternal(aCmdLine, uriFlag);
      if (uri)
        uris.push(uri);
    }

    for (let i = 0; i < aCmdLine.length; i++) {
      let arg = aCmdLine.getArgument(i);
      if (!arg || arg[0] == '-')
        continue;

      let uri = resolveURIInternal(aCmdLine, arg);
      if (uri)
        uris.push(uri);
    }

    
    let browserWin;
    try {
      let localeWin = Services.wm.getMostRecentWindow("navigator:localepicker");
      if (localeWin) {
        localeWin.focus();
        aCmdLine.preventDefault = true;
        return;
      }

      browserWin = Services.wm.getMostRecentWindow("navigator:browser");
      if (!browserWin) {
        
        let defaultURL = getHomePage();

        
        if (uris.length > 0) {
          defaultURL = uris[0].spec;
          uris = uris.slice(1);
        }

        
        let showLocalePicker = Services.prefs.getBoolPref("browser.firstrun.show.localepicker");
        if ((needHomepageOverride() == "new profile" && showLocalePicker && !haveSystemLocale())) { 
          browserWin = openWindow(null, "chrome://browser/content/localePicker.xul", "_blank", "chrome,dialog=no,all", defaultURL);
          aCmdLine.preventDefault = true;
          return;
        }

        browserWin = openWindow(null, "chrome://browser/content/browser.xul", "_blank", "chrome,dialog=no,all", defaultURL);
      }

      browserWin.focus();

      
      aCmdLine.preventDefault = true;
    } catch (e) {
      Cu.reportError(e);
    }

    
    

    
    while (!browserWin.browserDOMWindow)
      Services.tm.currentThread.processNextEvent(true);

    
    for (let i = 0; i < uris.length; i++)
      browserWin.browserDOMWindow.openURI(uris[i], null, Ci.nsIBrowserDOMWindow.OPEN_NEWTAB, Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL);

    if (appURI)
      browserWin.browserDOMWindow.openURI(appURI, null, browserWin.OPEN_APPTAB, Ci.nsIBrowserDOMWindow.OPEN_NEW);

    
    if (alertFlag) {
      if (alertFlag == "update-app") {
        
        Services.prefs.setBoolPref("app.update.skipNotification", true);

        var updateService = Cc["@mozilla.org/updates/update-service;1"].getService(Ci.nsIApplicationUpdateService);
        var updateTimerCallback = updateService.QueryInterface(Ci.nsITimerCallback);
        updateTimerCallback.notify(null);
      } else if (alertFlag.length >= 9 && alertFlag.substr(0, 9) == "download:") {
        showPanelWhenReady(browserWin, "downloads-container");
      } else if (alertFlag == "addons") {
        showPanelWhenReady(browserWin, "addons-container");
      }
    }
  },

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler]),

  
  classID: Components.ID("{be623d20-d305-11de-8a39-0800200c9a66}"),
};

var components = [ BrowserCLH ];
const NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
