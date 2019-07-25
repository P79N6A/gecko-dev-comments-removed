




































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");


function openWindow(aParent, aURL, aTarget, aFeatures) {
  let wwatch = Cc["@mozilla.org/embedcomp/window-watcher;1"].getService(Ci.nsIWindowWatcher);
  return wwatch.openWindow(aParent, aURL, aTarget, aFeatures, null);
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


function BrowserCLH() { }

BrowserCLH.prototype = {
  
  
  
  handle: function fs_handle(aCmdLine) {
    
    
    
    
    
    
    if (aCmdLine.findFlag("silent", false) > -1) {
      let searchService = Cc["@mozilla.org/browser/search-service;1"].
                          getService(Ci.nsIBrowserSearchService);
      let autoComplete = Cc["@mozilla.org/autocomplete/search;1?name=history"].
                         getService(Ci.nsIAutoCompleteSearch);
    }

    
    let chromeParam = aCmdLine.handleFlagWithParam("chrome", false);
    if (chromeParam) {
      try {
        
        let features = "chrome,dialog=no,all";
        let uri = resolveURIInternal(aCmdLine, chromeParam);
        let netutil = Cc["@mozilla.org/network/util;1"].getService(Ci.nsINetUtil);
        if (!netutil.URIChainHasFlags(uri, Ci.nsIHttpProtocolHandler.URI_INHERITS_SECURITY_CONTEXT)) {
          openWindow(null, uri.spec, "_blank", features);
          aCmdLine.preventDefault = true;
        }
      }
      catch (e) {
        Cu.reportError(e);
      }
    }

    let win;
    try {
      let windowMediator = Cc["@mozilla.org/appshell/window-mediator;1"].getService(Ci.nsIWindowMediator);
      win = windowMediator.getMostRecentWindow("navigator:browser");
      if (!win)
        return;

      win.focus();
      aCmdLine.preventDefault = true;
    } catch (e) { }

    
    
    for (let i = 0; i < aCmdLine.length; i++) {
      let arg = aCmdLine.getArgument(i);
      if (!arg || arg[0] == '-')
        continue;

      let uri = resolveURIInternal(aCmdLine, arg);
      if (uri)
        win.browserDOMWindow.openURI(uri, null, Ci.nsIBrowserDOMWindow.OPEN_NEWTAB, null);
    }
  },

  
  QueryInterface: XPCOMUtils.generateQI([Ci.nsICommandLineHandler]),

  
  classID: Components.ID("{be623d20-d305-11de-8a39-0800200c9a66}"),
};

var components = [ BrowserCLH ];
const NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
