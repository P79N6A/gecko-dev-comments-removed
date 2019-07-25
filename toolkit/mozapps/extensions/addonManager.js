












































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
  observe: function AMC_observe(subject, topic, data) {
    let os = Cc["@mozilla.org/observer-service;1"].
             getService(Ci.nsIObserverService);

    switch (topic) {
    case "profile-after-change":
      os.addObserver(this, "xpcom-shutdown", false);
      AddonManagerPrivate.startup();
      break;
    case "xpcom-shutdown":
      os.removeObserver(this, "xpcom-shutdown");
      AddonManagerPrivate.shutdown();
      break;
    }
  },

  


  isInstallEnabled: function AMC_isInstallEnabled(mimetype, referer) {
    return AddonManager.isInstallEnabled(mimetype);
  },

  


  installAddonsFromWebpage: function AMC_installAddonsFromWebpage(mimetype,
                                                                  window,
                                                                  referer, uris,
                                                                  hashes, names,
                                                                  icons, callback) {
    if (uris.length == 0)
      return false;

    let retval = true;
    if (!AddonManager.isInstallAllowed(mimetype, referer)) {
      callback = null;
      retval = false;
    }

    let loadGroup = null;

    try {
      loadGroup = window.QueryInterface(Ci.nsIInterfaceRequestor)
                        .getInterface(Ci.nsIWebNavigation)
                        .QueryInterface(Ci.nsIDocumentLoader).loadGroup;
    }
    catch (e) {
    }

    let installs = [];
    function buildNextInstall() {
      if (uris.length == 0) {
        AddonManager.installAddonsFromWebpage(mimetype, window, referer, installs);
        return;
      }
      let uri = uris.shift();
      AddonManager.getInstallForURL(uri, function(install) {
        if (install) {
          installs.push(install);
          if (callback) {
            install.addListener({
              onDownloadCancelled: function(install) {
                callback.onInstallEnded(uri, USER_CANCELLED);
              },

              onDownloadFailed: function(install, error) {
                if (error == AddonManager.ERROR_CORRUPT_FILE)
                  callback.onInstallEnded(uri, CANT_READ_ARCHIVE);
                else
                  callback.onInstallEnded(uri, DOWNLOAD_ERROR);
              },

              onInstallFailed: function(install, error) {
                callback.onInstallEnded(uri, EXECUTION_ERROR);
              },

              onInstallEnded: function(install, status) {
                callback.onInstallEnded(uri, SUCCESS);
              }
            });
          }
        }
        else if (callback) {
          callback.callback(uri, UNSUPPORTED_TYPE);
        }
        buildNextInstall();
      }, mimetype, hashes.shift(), names.shift(), icons.shift(), null, loadGroup);
    }
    buildNextInstall();

    return retval;
  },

  notify: function AMC_notify(timer) {
    AddonManagerPrivate.backgroundUpdateCheck();
  },

  classDescription: "Addons Manager",
  contractID: "@mozilla.org/addons/integration;1",
  classID: Components.ID("{4399533d-08d1-458c-a87a-235f74451cfa}"),
  _xpcom_categories: [{ category: "profile-after-change" },
                      { category: "update-timer",
                        value: "@mozilla.org/addons/integration;1," +
                               "getService,addon-background-update-timer," +
                               PREF_EM_UPDATE_INTERVAL + ",86400" }],
  _xpcom_factory: {
    createInstance: function(outer, iid) {
      if (outer != null)
        throw Cr.NS_ERROR_NO_AGGREGATION;
  
      if (!gSingleton)
        gSingleton = new amManager();
      return gSingleton.QueryInterface(iid);
    }
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.amIWebInstaller,
                                         Ci.nsITimerCallback,
                                         Ci.nsIObserver])
};

function NSGetModule(compMgr, fileSpec)
  XPCOMUtils.generateModule([amManager]);
