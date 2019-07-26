



this.EXPORTED_SYMBOLS = ["webappsUI"];

let Ci = Components.interfaces;
let Cc = Components.classes;
let Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/WebappsInstaller.jsm");
Cu.import("resource://gre/modules/WebappOSUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

this.webappsUI = {
  downloads: {},

  init: function webappsUI_init() {
    Services.obs.addObserver(this, "webapps-ask-install", false);
    Services.obs.addObserver(this, "webapps-launch", false);
    Services.obs.addObserver(this, "webapps-uninstall", false);
    cpmm.addMessageListener("Webapps:OfflineCache", this);
  },

  uninit: function webappsUI_uninit() {
    Services.obs.removeObserver(this, "webapps-ask-install");
    Services.obs.removeObserver(this, "webapps-launch");
    Services.obs.removeObserver(this, "webapps-uninstall");
    cpmm.removeMessageListener("Webapps:OfflineCache", this);
  },

  receiveMessage: function(aMessage) {
    let data = aMessage.data;

    if (aMessage.name == "Webapps:OfflineCache" &&
        data.installState == "installed" &&
        this.downloads[data.manifest]) {
      this.downloads[data.manifest].resolve();
    }
  },

  observe: function webappsUI_observe(aSubject, aTopic, aData) {
    let data = JSON.parse(aData);
    data.mm = aSubject;

    switch(aTopic) {
      case "webapps-ask-install":
        let win = this._getWindowForId(data.oid);
        if (win && win.location.href == data.from) {
          this.doInstall(data, win);
        }
        break;
      case "webapps-launch":
        WebappOSUtils.launch(data);
        break;
      case "webapps-uninstall":
        WebappOSUtils.uninstall(data);
        break;
    }
  },

  _getWindowForId: function(aId) {
    let someWindow = Services.wm.getMostRecentWindow(null);
    return someWindow && Services.wm.getOuterWindowWithId(aId);
  },

  doInstall: function(aData, aWindow) {
    let browser = aWindow.QueryInterface(Ci.nsIInterfaceRequestor)
                         .getInterface(Ci.nsIWebNavigation)
                         .QueryInterface(Ci.nsIDocShell)
                         .chromeEventHandler;
    let chromeDoc = browser.ownerDocument;
    let chromeWin = chromeDoc.defaultView;
    let popupProgressContent =
      chromeDoc.getElementById("webapps-install-progress-content");

    let bundle = chromeWin.gNavigatorBundle;

    let notification;

    let mainAction = {
      label: bundle.getString("webapps.install"),
      accessKey: bundle.getString("webapps.install.accesskey"),
      callback: () => {
        notification.remove();

        notification = chromeWin.PopupNotifications.
                        show(browser,
                             "webapps-install-progress",
                             bundle.getString("webapps.install.inprogress"),
                             "webapps-notification-icon");

        let progressMeter = chromeDoc.createElement("progressmeter");
        progressMeter.setAttribute("mode", "undetermined");
        popupProgressContent.appendChild(progressMeter);

        let manifestURL = aData.app.manifestURL;
        if (aData.app.manifest && aData.app.manifest.appcache_path) {
          this.downloads[manifestURL] = Promise.defer();
        }

        let app = WebappsInstaller.init(aData);

        if (app) {
          let localDir = null;
          if (app.appProfile) {
            localDir = app.appProfile.localDir;
          }

          DOMApplicationRegistry.confirmInstall(aData, localDir,
            (aManifest, aZipPath) => {
              Task.spawn(function() {
                try {
                  yield WebappsInstaller.install(aData, aManifest, aZipPath);
                  if (this.downloads[manifestURL]) {
                    yield this.downloads[manifestURL].promise;
                  }
                  installationSuccessNotification(aData, app, bundle);
                } catch (ex) {
                  Cu.reportError("Error installing webapp: " + ex);
                  
                } finally {
                  popupProgressContent.removeChild(progressMeter);
                  notification.remove();
                  delete this.downloads[manifestURL];
                }
              }.bind(this));
            });
        } else {
          DOMApplicationRegistry.denyInstall(aData);
        }
      }
    };

    let requestingURI = chromeWin.makeURI(aData.from);
    let jsonManifest = aData.isPackage ? aData.app.updateManifest : aData.app.manifest;
    let manifest = new ManifestHelper(jsonManifest, aData.app.origin);

    let host;
    try {
      host = requestingURI.host;
    } catch(e) {
      host = requestingURI.spec;
    }

    let message = bundle.getFormattedString("webapps.requestInstall",
                                            [manifest.name, host], 2);

    notification = chromeWin.PopupNotifications.show(browser,
                                                     "webapps-install",
                                                     message,
                                                     "webapps-notification-icon",
                                                     mainAction);

  }
}

function installationSuccessNotification(aData, app, aBundle) {
  let launcher = {
    observe: function(aSubject, aTopic) {
      if (aTopic == "alertclickcallback") {
        WebappOSUtils.launch(aData.app);
      }
    }
  };

  try {
    let notifier = Cc["@mozilla.org/alerts-service;1"].
                   getService(Ci.nsIAlertsService);

    notifier.showAlertNotification(app.iconURI.spec,
                                   aBundle.getString("webapps.install.success"),
                                   app.appNameAsFilename,
                                   true, null, launcher);
  } catch (ex) {}
}
