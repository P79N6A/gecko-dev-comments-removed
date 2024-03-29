









"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

const PREF_EM_UPDATE_INTERVAL = "extensions.update.interval";


const EXECUTION_ERROR   = -203;
const CANT_READ_ARCHIVE = -207;
const USER_CANCELLED    = -210;
const DOWNLOAD_ERROR    = -228;
const UNSUPPORTED_TYPE  = -244;
const SUCCESS           = 0;

const MSG_INSTALL_ENABLED  = "WebInstallerIsInstallEnabled";
const MSG_INSTALL_ADDONS   = "WebInstallerInstallAddonsFromWebpage";
const MSG_INSTALL_CALLBACK = "WebInstallerInstallCallback";

const CHILD_SCRIPT = "resource://gre/modules/addons/Content.js";

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

let gSingleton = null;

let gParentMM = null;


function amManager() {
  Cu.import("resource://gre/modules/AddonManager.jsm");

  let globalMM = Cc["@mozilla.org/globalmessagemanager;1"]
                 .getService(Ci.nsIMessageListenerManager);
  globalMM.loadFrameScript(CHILD_SCRIPT, true);
  globalMM.addMessageListener(MSG_INSTALL_ADDONS, this);

  gParentMM = Cc["@mozilla.org/parentprocessmessagemanager;1"]
                 .getService(Ci.nsIMessageListenerManager);
  gParentMM.addMessageListener(MSG_INSTALL_ENABLED, this);
}

amManager.prototype = {
  observe: function AMC_observe(aSubject, aTopic, aData) {
    if (aTopic == "addons-startup")
      AddonManagerPrivate.startup();
  },

  


  mapURIToAddonID: function AMC_mapURIToAddonID(uri, id) {
    id.value = AddonManager.mapURIToAddonID(uri);
    return !!id.value;
  },

  


  isInstallEnabled: function AMC_isInstallEnabled(aMimetype, aReferer) {
    return AddonManager.isInstallEnabled(aMimetype);
  },

  


  installAddonsFromWebpage: function AMC_installAddonsFromWebpage(aMimetype,
                                                                  aBrowser,
                                                                  aReferer, aUris,
                                                                  aHashes, aNames,
                                                                  aIcons, aCallback) {
    if (aUris.length == 0)
      return false;

    let retval = true;
    if (!AddonManager.isInstallAllowed(aMimetype, aReferer)) {
      aCallback = null;
      retval = false;
    }

    let installs = [];
    function buildNextInstall() {
      if (aUris.length == 0) {
        AddonManager.installAddonsFromWebpage(aMimetype, aBrowser, aReferer, installs);
        return;
      }
      let uri = aUris.shift();
      AddonManager.getInstallForURL(uri, function buildNextInstall_getInstallForURL(aInstall) {
        function callCallback(aUri, aStatus) {
          try {
            aCallback.onInstallEnded(aUri, aStatus);
          }
          catch (e) {
            Components.utils.reportError(e);
          }
        }

        if (aInstall) {
          installs.push(aInstall);
          if (aCallback) {
            aInstall.addListener({
              onDownloadCancelled: function buildNextInstall_onDownloadCancelled(aInstall) {
                callCallback(uri, USER_CANCELLED);
              },

              onDownloadFailed: function buildNextInstall_onDownloadFailed(aInstall) {
                if (aInstall.error == AddonManager.ERROR_CORRUPT_FILE)
                  callCallback(uri, CANT_READ_ARCHIVE);
                else
                  callCallback(uri, DOWNLOAD_ERROR);
              },

              onInstallFailed: function buildNextInstall_onInstallFailed(aInstall) {
                callCallback(uri, EXECUTION_ERROR);
              },

              onInstallEnded: function buildNextInstall_onInstallEnded(aInstall, aStatus) {
                callCallback(uri, SUCCESS);
              }
            });
          }
        }
        else if (aCallback) {
          aCallback.onInstallEnded(uri, UNSUPPORTED_TYPE);
        }
        buildNextInstall();
      }, aMimetype, aHashes.shift(), aNames.shift(), aIcons.shift(), null, aBrowser);
    }
    buildNextInstall();

    return retval;
  },

  notify: function AMC_notify(aTimer) {
    AddonManagerPrivate.backgroundUpdateTimerHandler();
  },

  





  receiveMessage: function AMC_receiveMessage(aMessage) {
    let payload = aMessage.data;
    let referer = payload.referer ? Services.io.newURI(payload.referer, null, null)
                                  : null;

    switch (aMessage.name) {
      case MSG_INSTALL_ENABLED:
        return this.isInstallEnabled(payload.mimetype, referer);

      case MSG_INSTALL_ADDONS: {
        let callback = null;
        if (payload.callbackID != -1) {
          callback = {
            onInstallEnded: function ITP_callback(url, status) {
              gParentMM.broadcastAsyncMessage(MSG_INSTALL_CALLBACK, {
                callbackID: payload.callbackID,
                url: url,
                status: status
              });
            },
          };
        }

        return this.installAddonsFromWebpage(payload.mimetype,
          aMessage.target, referer, payload.uris, payload.hashes,
          payload.names, payload.icons, callback);
      }
    }
  },

  classID: Components.ID("{4399533d-08d1-458c-a87a-235f74451cfa}"),
  _xpcom_factory: {
    createInstance: function AMC_createInstance(aOuter, aIid) {
      if (aOuter != null)
        throw Components.Exception("Component does not support aggregation",
                                   Cr.NS_ERROR_NO_AGGREGATION);

      if (!gSingleton)
        gSingleton = new amManager();
      return gSingleton.QueryInterface(aIid);
    }
  },
  QueryInterface: XPCOMUtils.generateQI([Ci.amIAddonManager,
                                         Ci.amIWebInstaller,
                                         Ci.nsITimerCallback,
                                         Ci.nsIObserver,
                                         Ci.nsIMessageListener])
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([amManager]);
