



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SocialService", "resource://gre/modules/SocialService.jsm");

const EXPORTED_SYMBOLS = ["MozSocialAPI"];

var MozSocialAPI = {
  _enabled: false,
  _everEnabled: false,
  set enabled(val) {
    let enable = !!val;
    if (enable == this._enabled) {
      return;
    }
    this._enabled = enable;

    if (enable) {
      Services.obs.addObserver(injectController, "document-element-inserted", false);

      if (!this._everEnabled) {
        this._everEnabled = true;
        Services.telemetry.getHistogramById("SOCIAL_ENABLED_ON_SESSION").add(true);
      }

    } else {
      Services.obs.removeObserver(injectController, "document-element-inserted", false);
    }
  }
};



function injectController(doc, topic, data) {
  try {
    let window = doc.defaultView;
    if (!window)
      return;

    var containingBrowser = window.QueryInterface(Ci.nsIInterfaceRequestor)
                                  .getInterface(Ci.nsIWebNavigation)
                                  .QueryInterface(Ci.nsIDocShell)
                                  .chromeEventHandler;

    let origin = containingBrowser.getAttribute("origin");
    if (!origin) {
      return;
    }

    SocialService.getProvider(origin, function(provider) {
      if (provider && provider.workerURL) {
        attachToWindow(provider, window);
      }
    });
  } catch(e) {
    Cu.reportError("MozSocialAPI injectController: unable to attachToWindow for " + doc.location + ": " + e);
  }
}


function attachToWindow(provider, targetWindow) {
  let origin = provider.origin;
  if (!provider.enabled) {
    throw new Error("MozSocialAPI: cannot attach disabled provider " + origin);
  }

  let targetDocURI = targetWindow.document.documentURIObject;
  if (provider.origin != targetDocURI.prePath) {
    throw new Error("MozSocialAPI: cannot attach " + origin + " to " + targetDocURI.spec);
  }

  var port = provider._getWorkerPort(targetWindow);

  let mozSocialObj = {
    
    
    getWorker: {
      enumerable: true,
      configurable: true,
      writable: true,
      value: function() {
        return {
          port: port,
          __exposedProps__: {
            port: "r"
          }
        };
      }
    },
    hasBeenIdleFor: {
      enumerable: true,
      configurable: true,
      writable: true,
      value: function() {
        return false;
      }
    },
    openServiceWindow: {
      enumerable: true,
      configurable: true,
      writable: true,
      value: function(toURL, name, options) {
        let url = targetWindow.document.documentURIObject.resolve(toURL);
        return openServiceWindow(provider, targetWindow, url, name, options);
      }
    },
    openChatWindow: {
      enumerable: true,
      configurable: true,
      writable: true,
      value: function(toURL, callback) {
        let url = targetWindow.document.documentURIObject.resolve(toURL);
        openChatWindow(getChromeWindow(targetWindow), provider, url, callback);
      }
    },
    openPanel: {
      enumerable: true,
      configurable: true,
      writable: true,
      value: function(toURL, offset, callback) {
        let chromeWindow = getChromeWindow(targetWindow);
        if (!chromeWindow.SocialFlyout)
          return;
        let url = targetWindow.document.documentURIObject.resolve(toURL);
        let fullURL = ensureProviderOrigin(provider, url);
        if (!fullURL)
          return;
        chromeWindow.SocialFlyout.open(fullURL, offset, callback);
      }
    },
    getAttention: {
      enumerable: true,
      configurable: true,
      writable: true,
      value: function() {
        getChromeWindow(targetWindow).getAttention();
      }
    },
    isVisible: {
      enumerable: true,
      configurable: true,
      get: function() {
        return targetWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                           .getInterface(Ci.nsIWebNavigation)
                           .QueryInterface(Ci.nsIDocShell).isActive;
      }
    }
  };

  let contentObj = Cu.createObjectIn(targetWindow);
  Object.defineProperties(contentObj, mozSocialObj);
  Cu.makeObjectPropsNormal(contentObj);

  targetWindow.navigator.wrappedJSObject.__defineGetter__("mozSocial", function() {
    
    
    
    
    delete targetWindow.navigator.wrappedJSObject.mozSocial;
    return targetWindow.navigator.wrappedJSObject.mozSocial = contentObj;
  });

  targetWindow.addEventListener("unload", function () {
    
    
    
    schedule(function () { port.close(); });
  });
}

function schedule(callback) {
  Services.tm.mainThread.dispatch(callback, Ci.nsIThread.DISPATCH_NORMAL);
}

function getChromeWindow(contentWin) {
  return contentWin.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIWebNavigation)
                   .QueryInterface(Ci.nsIDocShellTreeItem)
                   .rootTreeItem
                   .QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindow);
}

function ensureProviderOrigin(provider, url) {
  
  let uri;
  let fullURL;
  try {
    fullURL = Services.io.newURI(provider.origin, null, null).resolve(url);
    uri = Services.io.newURI(fullURL, null, null);
  } catch (ex) {
    Cu.reportError("openServiceWindow: failed to resolve window URL: " + url + "; " + ex);
    return null;
  }

  if (provider.origin != uri.prePath) {
    Cu.reportError("openServiceWindow: unable to load new location, " +
                   provider.origin + " != " + uri.prePath);
    return null;
  }
  return fullURL;
}

function openChatWindow(chromeWindow, provider, url, callback) {
  if (!chromeWindow.SocialChatBar)
    return;
  let fullURL = ensureProviderOrigin(provider, url);
  if (!fullURL)
    return;
  chromeWindow.SocialChatBar.newChat(provider, fullURL, callback);
}

function openServiceWindow(provider, contentWindow, url, name, options) {
  
  let fullURL = ensureProviderOrigin(provider, url);
  if (!fullURL)
    return null;

  let windowName = provider.origin + name;
  let chromeWindow = Services.ww.getWindowByName(windowName, null);
  let tabbrowser = chromeWindow && chromeWindow.gBrowser;
  if (tabbrowser &&
      tabbrowser.selectedBrowser.getAttribute("origin") == provider.origin) {
    return tabbrowser.contentWindow;
  }

  let serviceWindow = contentWindow.openDialog(fullURL, windowName,
                                               "chrome=no,dialog=no" + options);

  
  chromeWindow = getChromeWindow(serviceWindow);

  
  
  chromeWindow.name = windowName;
  chromeWindow.gBrowser.selectedBrowser.setAttribute("origin", provider.origin);

  
  chromeWindow.gBrowser.docShell.QueryInterface(Components.interfaces.nsIDocShellHistory).useGlobalHistory = false;

  
  
  serviceWindow.addEventListener("DOMTitleChanged", function() {
    let sep = xulWindow.document.documentElement.getAttribute("titlemenuseparator");
    xulWindow.document.title = provider.name + sep + serviceWindow.document.title;
  });

  return serviceWindow;
}
