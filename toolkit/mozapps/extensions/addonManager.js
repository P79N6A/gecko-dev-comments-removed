












































const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const PREF_EM_UPDATE_INTERVAL = "extensions.update.interval";


const EXECUTION_ERROR   = -203;
const CANT_READ_ARCHIVE = -207;
const USER_CANCELLED    = -210;
const DOWNLOAD_ERROR    = -228;
const UNSUPPORTED_TYPE  = -244;
const SUCCESS = 0;

Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

var gSingleton = null;

function amManager() {
  Components.utils.import("resource://gre/modules/AddonManager.jsm");
}

amManager.prototype = {
  observe: function AMC_observe(aSubject, aTopic, aData) {
    let os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);

    switch (aTopic) {
    case "addons-startup":
      os.addObserver(this, "xpcom-shutdown", false);
      AddonManagerPrivate.startup();
      break;
    case "xpcom-shutdown":
      os.removeObserver(this, "xpcom-shutdown");
      AddonManagerPrivate.shutdown();
      break;
    }
  },

  


  isInstallEnabled: function AMC_isInstallEnabled(aMimetype, aReferer) {
    return AddonManager.isInstallEnabled(aMimetype);
  },

  


  installAddonsFromWebpage: function AMC_installAddonsFromWebpage(aMimetype,
                                                                  aWindow,
                                                                  aReferer, aUris,
                                                                  aHashes, aNames,
                                                                  aIcons, aCallback) {
    if (aUris.length == 0)
      return false;

    if (!AddonManager.isInstallEnabled(aMimetype))
      return false;

    let retval = true;
    if (!AddonManager.isInstallAllowed(aMimetype, aReferer)) {
      aCallback = null;
      retval = false;
    }

    let loadGroup = null;

    try {
      loadGroup = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIWebNavigation)
                         .QueryInterface(Ci.nsIDocumentLoader).loadGroup;
    }
    catch (e) {
    }

    let installs = [];
    function buildNextInstall() {
      if (aUris.length == 0) {
        AddonManager.installAddonsFromWebpage(aMimetype, aWindow, aReferer, installs);
        return;
      }
      let uri = aUris.shift();
      AddonManager.getInstallForURL(uri, function(aInstall) {
        if (aInstall) {
          installs.push(aInstall);
          if (aCallback) {
            aInstall.addListener({
              onDownloadCancelled: function(aInstall) {
                aCallback.onInstallEnded(uri, USER_CANCELLED);
              },

              onDownloadFailed: function(aInstall) {
                if (aInstall.error == AddonManager.ERROR_CORRUPT_FILE)
                  aCallback.onInstallEnded(uri, CANT_READ_ARCHIVE);
                else
                  aCallback.onInstallEnded(uri, DOWNLOAD_ERROR);
              },

              onInstallFailed: function(aInstall) {
                aCallback.onInstallEnded(uri, EXECUTION_ERROR);
              },

              onInstallEnded: function(aInstall, aStatus) {
                aCallback.onInstallEnded(uri, SUCCESS);
              }
            });
          }
        }
        else if (aCallback) {
          aCallback.onInstallEnded(uri, UNSUPPORTED_TYPE);
        }
        buildNextInstall();
      }, aMimetype, aHashes.shift(), aNames.shift(), aIcons.shift(), null, loadGroup);
    }
    buildNextInstall();

    return retval;
  },

  notify: function AMC_notify(aTimer) {
    AddonManagerPrivate.backgroundUpdateCheck();
  },

  classID: Components.ID("{4399533d-08d1-458c-a87a-235f74451cfa}"),
  _xpcom_factory: {
    createInstance: function(aOuter, aIid) {
      if (aOuter != null)
        throw Cr.NS_ERROR_NO_AGGREGATION;
  
      if (!gSingleton)
        gSingleton = new amManager();
      return gSingleton.QueryInterface(aIid);
    }
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstaller,
                                         Ci.nsITimerCallback,
                                         Ci.nsIObserver])
};

var NSGetFactory = XPCOMUtils.generateNSGetFactory([amManager]);
