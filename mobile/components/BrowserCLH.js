




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

function openWindow(aParent, aURL, aTarget, aFeatures, aArgs) {
  let argString = null;
  if (aArgs) {
    argString = Cc["@mozilla.org/supports-string;1"].createInstance(Ci.nsISupportsString);
    argString.data = aArgs;
  }

  return Services.ww.openWindow(aParent, aURL, aTarget, aFeatures, argString);
}

function resolveURIInternal(aCmdLine, aArgument) {
  let uri = aCmdLine.resolveURI(aArgument);

  if (!(uri instanceof Ci.nsIFileURL))
    return uri;

  try {
    if (uri.file.exists())
      return uri;
  }
  catch (e) {
    Cu.reportError(e);
  }

  try {
    let urifixup = Cc["@mozilla.org/docshell/urifixup;1"].getService(Ci.nsIURIFixup);
    uri = urifixup.createFixupURI(aArgument, 0);
  }
  catch (e) {
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
      }
      catch (e) {
        Cu.reportError(e);
      }
      return;
    }

    
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

    
    let win;
    try {
      win = Services.wm.getMostRecentWindow("navigator:browser");
      if (!win) {
        
        let defaultURL = getHomePage();

        
        if (needHomepageOverride() == "new profile")
            defaultURL = "about:firstrun";

        
        if (uris.length > 0) {
          defaultURL = uris[0].spec;
          uris = uris.slice(1);
        }

        win = openWindow(null, "chrome://browser/content/browser.xul", "_blank", "chrome,dialog=no,all", defaultURL);
      }

      win.focus();

      
      aCmdLine.preventDefault = true;
    } catch (e) { }

    
    
    if (uris.length == 0)
      return;

    
    while (!win.browserDOMWindow)
      Services.tm.currentThread.processNextEvent(true);

    
    for (let i = 0; i < uris.length; i++)
      win.browserDOMWindow.openURI(uris[i], null, Ci.nsIBrowserDOMWindow.OPEN_NEWTAB, Ci.nsIBrowserDOMWindow.OPEN_EXTERNAL);
  },

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler]),

  
  classID: Components.ID("{be623d20-d305-11de-8a39-0800200c9a66}"),
};

var components = [ BrowserCLH ];
const NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
