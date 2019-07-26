






const URI_EXTENSION_UPDATE_DIALOG = "chrome://mozapps/content/extensions/update.xul";

const PREF_GETADDONS_BYIDS            = "extensions.getAddons.get.url";
const PREF_MIN_PLATFORM_COMPAT        = "extensions.minCompatiblePlatformVersion";

Components.utils.import("resource://gre/modules/Promise.jsm");

let repo = {};
let ARContext = Components.utils.import("resource://gre/modules/AddonRepository.jsm", repo);
info("ARContext: " + Object.keys(ARContext).join(", "));



let pXHRStarted = Promise.defer();
let oldXHRConstructor = ARContext.XHRequest;
ARContext.XHRequest = function() {
  this._handlers = new Map();
  this.mozBackgroundRequest = false;
  this.timeout = undefined;
  this.open = function(aMethod, aURI, aAsync) {
      this.method = aMethod;
      this.uri = aURI;
      this.async = aAsync;
      info("Opened XHR for " + aMethod + " " + aURI);
    };
  this.overrideMimeType = function(aMimeType) {
      this.mimeType = aMimeType;
    };
  this.addEventListener = function(aEvent, aHandler, aCapture) {
      this._handlers.set(aEvent, aHandler);
    };
  this.send = function(aBody) {
      info("Send XHR for " + this.method + " " + this.uri + " handlers: " + [this._handlers.keys()].join(", "));
      pXHRStarted.resolve(this);
    }
};




function promise_open_compatibility_window(aInactiveAddonIds) {
  let deferred = Promise.defer();
  
  
  
  requestLongerTimeout(100 );

  var variant = Cc["@mozilla.org/variant;1"].
                createInstance(Ci.nsIWritableVariant);
  variant.setFromVariant(aInactiveAddonIds);

  
  
  var features = "chrome,centerscreen,dialog,titlebar";
  var ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  var win = ww.openWindow(null, URI_EXTENSION_UPDATE_DIALOG, "", features, variant);

  win.addEventListener("load", function() {
    function page_shown(aEvent) {
      if (aEvent.target.pageid)
        info("Page " + aEvent.target.pageid + " shown");
    }

    win.removeEventListener("load", arguments.callee, false);

    info("Compatibility dialog opened");

    win.addEventListener("pageshow", page_shown, false);
    win.addEventListener("unload", function() {
      win.removeEventListener("unload", arguments.callee, false);
      win.removeEventListener("pageshow", page_shown, false);
      dump("Compatibility dialog closed\n");
    }, false);

    deferred.resolve(win);
  }, false);
  return deferred.promise;
}

function promise_window_close(aWindow) {
  let deferred = Promise.defer();
  aWindow.addEventListener("unload", function() {
    aWindow.removeEventListener("unload", arguments.callee, false);
    deferred.resolve(aWindow);
  }, false);
  return deferred.promise;
}



add_task(function* amo_ping_timeout() {
  Services.prefs.setBoolPref(PREF_GETADDONS_CACHE_ENABLED, true);
  let compatWindow = yield promise_open_compatibility_window([]);

  let xhr = yield pXHRStarted.promise;
  is(xhr.timeout, 30000, "XHR request should have 30 second timeout");
  ok(xhr._handlers.has("timeout"), "Timeout handler set on XHR");
  
  xhr._handlers.get("timeout")();

  
  ARContext.XHRequest = oldXHRConstructor;
  
  yield promise_window_close(compatWindow);
});
