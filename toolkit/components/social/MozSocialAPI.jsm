



const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "SocialService", "resource://gre/modules/SocialService.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PrivateBrowsingUtils", "resource://gre/modules/PrivateBrowsingUtils.jsm");

this.EXPORTED_SYMBOLS = ["MozSocialAPI", "openChatWindow"];

this.MozSocialAPI = {
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
    if (!window || PrivateBrowsingUtils.isWindowPrivate(window))
      return;

    
    if (doc.documentURIObject.scheme == "about") {
      return;
    }

    var containingBrowser = window.QueryInterface(Ci.nsIInterfaceRequestor)
                                  .getInterface(Ci.nsIWebNavigation)
                                  .QueryInterface(Ci.nsIDocShell)
                                  .chromeEventHandler;

    let origin = containingBrowser.getAttribute("origin");
    if (!origin) {
      return;
    }

    SocialService.getProvider(origin, function(provider) {
      if (provider && provider.workerURL && provider.enabled) {
        attachToWindow(provider, window);
      }
    });
  } catch(e) {
    Cu.reportError("MozSocialAPI injectController: unable to attachToWindow for " + doc.location + ": " + e);
  }
}


function attachToWindow(provider, targetWindow) {
  
  
  let targetDocURI = targetWindow.document.documentURIObject;
  if (!provider.isSameOrigin(targetDocURI)) {
    let msg = "MozSocialAPI: not attaching mozSocial API for " + provider.origin +
              " to " + targetDocURI.spec + " since origins differ."
    Services.console.logStringMessage(msg);
    return;
  }

  var port = provider.getWorkerPort(targetWindow);

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
        if (!provider.isSameOrigin(url))
          return;
        chromeWindow.SocialFlyout.open(url, offset, callback);
      }
    },
    closePanel: {
      enumerable: true,
      configurable: true,
      writable: true,
      value: function(toURL, offset, callback) {
        let chromeWindow = getChromeWindow(targetWindow);
        if (!chromeWindow.SocialFlyout || !chromeWindow.SocialFlyout.panel)
          return;
        chromeWindow.SocialFlyout.panel.hidePopup();
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

  
  
  
  
  
  let dwu = targetWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIDOMWindowUtils);
  dwu.allowScriptsToClose();

  targetWindow.addEventListener("DOMWindowClose", function _mozSocialDOMWindowClose(evt) {
    let elt = targetWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                .getInterface(Ci.nsIWebNavigation)
                .QueryInterface(Ci.nsIDocShell)
                .chromeEventHandler;
    while (elt) {
      if (elt.nodeName == "panel") {
        elt.hidePopup();
        break;
      } else if (elt.nodeName == "chatbox") {
        elt.close();
        break;
      }
      elt = elt.parentNode;
    }
    
    
    
    
    
    
    
    evt.preventDefault();
  }, true);
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

function isWindowGoodForChats(win) {
  return win.SocialChatBar
         && win.SocialChatBar.isAvailable
         && !PrivateBrowsingUtils.isWindowPrivate(win);
}

function findChromeWindowForChats(preferredWindow) {
  if (preferredWindow && isWindowGoodForChats(preferredWindow))
    return preferredWindow;
  
  
  
  let topMost, enumerator;
  
  
  
  
  const BROKEN_WM_Z_ORDER = Services.appinfo.OS != "WINNT";
  if (BROKEN_WM_Z_ORDER) {
    
    enumerator = Services.wm.getEnumerator("navigator:browser");
  } else {
    
    
    enumerator = Services.wm.getZOrderDOMWindowEnumerator("navigator:browser", false);
  }
  while (enumerator.hasMoreElements()) {
    let win = enumerator.getNext();
    if (win && isWindowGoodForChats(win))
      topMost = win;
  }
  return topMost;
}

this.openChatWindow =
 function openChatWindow(chromeWindow, provider, url, callback, mode) {
  chromeWindow = findChromeWindowForChats(chromeWindow);
  if (!chromeWindow) {
    Cu.reportError("Failed to open a social chat window - no host window could be found.");
    return;
  }
  let fullURI = provider.resolveUri(url);
  if (!provider.isSameOrigin(fullURI)) {
    Cu.reportError("Failed to open a social chat window - the requested URL is not the same origin as the provider.");
    return;
  }
  if (!chromeWindow.SocialChatBar.openChat(provider, fullURI.spec, callback, mode)) {
    Cu.reportError("Failed to open a social chat window - the chatbar is not available in the target window.");
    return;
  }
  
  
  chromeWindow.getAttention();
}
