



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

    
    
    
    
    if (!(containingBrowser.id == "social-sidebar-browser" ||
          containingBrowser.id == "social-notification-browser")) {
      return;
    }

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
    
    
    getWorker: function() {
      return {
        port: port,
        __exposedProps__: {
          port: "r"
        }
      };
    },
    hasBeenIdleFor: function () {
      return false;
    }
  };

  let contentObj = Cu.createObjectIn(targetWindow);
  let propList = {};
  for (let prop in mozSocialObj) {
    propList[prop] = {
      enumerable: true,
      configurable: true,
      writable: true,
      value: mozSocialObj[prop]
    };
  }
  Object.defineProperties(contentObj, propList);
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
