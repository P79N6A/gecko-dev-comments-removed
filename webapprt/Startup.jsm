








this.EXPORTED_SYMBOLS = ["startup"];

const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");

Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/PermissionsInstaller.jsm");
Cu.import('resource://gre/modules/Payment.jsm');
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Promise.jsm");


Cu.import("resource://webapprt/modules/WebappsHandler.jsm");
Cu.import("resource://webapprt/modules/WebappRT.jsm");

function isFirstRunOrUpdate() {
  let savedBuildID = null;
  try {
    savedBuildID = Services.prefs.getCharPref("webapprt.buildID");
  } catch (e) {}

  let ourBuildID = Services.appinfo.platformBuildID;

  if (ourBuildID != savedBuildID) {
    Services.prefs.setCharPref("webapprt.buildID", ourBuildID);
    return true;
  }

  return false;
}



this.startup = function(window) {
  return Task.spawn(function () {
    
    let deferredRegistry = Promise.defer();
    function observeRegistryLoading() {
      Services.obs.removeObserver(observeRegistryLoading, "webapps-registry-start");
      deferredRegistry.resolve();
    }
    Services.obs.addObserver(observeRegistryLoading, "webapps-registry-start", false);

    
    
    let deferredWindowLoad = Promise.defer();
    if (window.document && window.document.getElementById("content")) {
      deferredWindowLoad.resolve();
    } else {
      window.addEventListener("load", function onLoad() {
        window.removeEventListener("load", onLoad, false);
        deferredWindowLoad.resolve();
      });
    }

    
    yield deferredRegistry.promise;

    
    let appID = Ci.nsIScriptSecurityManager.NO_APP_ID;
    let manifestURL = WebappRT.config.app.manifestURL;
    if (manifestURL) {
      appID = DOMApplicationRegistry.getAppLocalIdByManifestURL(manifestURL);

      
      
      
      if (isFirstRunOrUpdate(Services.prefs)) {
        PermissionsInstaller.installPermissions(WebappRT.config.app, true);
      }
    }

    
    yield deferredWindowLoad.promise;

    
    let appBrowser = window.document.getElementById("content");

    
    appBrowser.docShell.setIsApp(appID);
    appBrowser.setAttribute("src", WebappRT.launchURI);
  }).then(null, Cu.reportError.bind(Cu));
}
