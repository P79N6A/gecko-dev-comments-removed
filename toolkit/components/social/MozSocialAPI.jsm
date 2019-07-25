



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SocialService", "resource://gre/modules/SocialService.jsm");

const EXPORTED_SYMBOLS = ["MozSocialAPI"];

var MozSocialAPI = {
  _enabled: false,
  set enabled(val) {
    let enable = !!val;
    if (enable == this._enabled) {
      return;
    }
    this._enabled = enable;

    if (enable) {
      Services.obs.addObserver(injectController, "document-element-inserted", false);
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
        return openServiceWindow(provider, targetWindow, toURL, name, options);
      }
    },
    getAttention: {
      enumerable: true,
      configurable: true,
      writable: true,
      value: function() {
        let mainWindow = targetWindow.QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                           .getInterface(Components.interfaces.nsIWebNavigation)
                           .QueryInterface(Components.interfaces.nsIDocShellTreeItem)
                           .rootTreeItem
                           .QueryInterface(Components.interfaces.nsIInterfaceRequestor)
                           .getInterface(Components.interfaces.nsIDOMWindow);
        mainWindow.getAttention();
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

function openServiceWindow(provider, contentWindow, url, name, options) {
  
  let uri;
  let fullURL;
  try {
    fullURL = contentWindow.document.documentURIObject.resolve(url);
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

  function getChromeWindow(contentWin) {
    return contentWin.QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIWebNavigation)
                     .QueryInterface(Ci.nsIDocShellTreeItem)
                     .rootTreeItem
                     .QueryInterface(Ci.nsIInterfaceRequestor)
                     .getInterface(Ci.nsIDOMWindow);

  }
  let chromeWindow = Services.ww.getWindowByName("social-service-window-" + name,
                                                 getChromeWindow(contentWindow));
  let tabbrowser = chromeWindow && chromeWindow.gBrowser;
  if (tabbrowser &&
      tabbrowser.selectedBrowser.getAttribute("origin") == provider.origin) {
    return tabbrowser.contentWindow;
  }

  let serviceWindow = contentWindow.openDialog(fullURL, name,
                                               "chrome=no,dialog=no" + options);

  
  chromeWindow = getChromeWindow(serviceWindow);

  
  
  chromeWindow.name = "social-service-window-" + name;
  chromeWindow.gBrowser.selectedBrowser.setAttribute("origin", provider.origin);

  
  
  serviceWindow.addEventListener("DOMTitleChanged", function() {
    let sep = xulWindow.document.documentElement.getAttribute("titlemenuseparator");
    xulWindow.document.title = provider.name + sep + serviceWindow.document.title;
  });

  return serviceWindow;
}
