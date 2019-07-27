



"use strict";

this.EXPORTED_SYMBOLS = ["WebappManager"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

const UPDATE_URL_PREF = "browser.webapps.updateCheckUrl";

Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Promise.jsm");
Cu.import("resource://gre/modules/Task.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Notifications", "resource://gre/modules/Notifications.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "Messaging", "resource://gre/modules/Messaging.jsm");
XPCOMUtils.defineLazyModuleGetter(this, "PluralForm", "resource://gre/modules/PluralForm.jsm");

XPCOMUtils.defineLazyGetter(this, "Strings", function() {
  return Services.strings.createBundle("chrome://browser/locale/webapp.properties");
});

















function getFormattedPluralForm(stringName, formatterArgs, pluralNum) {
  
  let escapedArgs = [arg.replace(/;/g, String.fromCharCode(0x1B)) for (arg of formatterArgs)];
  let formattedString = Strings.formatStringFromName(stringName, escapedArgs, escapedArgs.length);
  let pluralForm = PluralForm.get(pluralNum, formattedString);
  let unescapedString = pluralForm.replace(String.fromCharCode(0x1B), ";", "g");
  return unescapedString;
}

let Log = Cu.import("resource://gre/modules/AndroidLog.jsm", {}).AndroidLog;
let debug = Log.d.bind(null, "WebappManager");

this.WebappManager = {
  __proto__: DOMRequestIpcHelper.prototype,

  get _testing() {
    try {
      return Services.prefs.getBoolPref("browser.webapps.testing");
    } catch(ex) {
      return false;
    }
  },

  install: function(aMessage, aMessageManager) {
    if (this._testing) {
      
      DOMApplicationRegistry.doInstall(aMessage, aMessageManager);
      return;
    }

    this._installApk(aMessage, aMessageManager);
  },

  installPackage: function(aMessage, aMessageManager) {
    if (this._testing) {
      
      DOMApplicationRegistry.doInstallPackage(aMessage, aMessageManager);
      return;
    }

    this._installApk(aMessage, aMessageManager);
  },

  _installApk: function(aMessage, aMessageManager) { return Task.spawn((function*() {
    if (this.inGuestSession()) {
      aMessage.error = Strings.GetStringFromName("webappsDisabledInGuest"),
      aMessageManager.sendAsyncMessage("Webapps:Install:Return:KO", aMessage);
      return;
    }

    let filePath;


    let appName = aMessage.app.manifest ? aMessage.app.manifest.name
                                        : aMessage.app.updateManifest.name;

    let downloadingNotification = this._notify({
      title: Strings.GetStringFromName("retrievingTitle"),
      message: Strings.formatStringFromName("retrievingMessage", [appName], 1),
      icon: "drawable://alert_download_animation",
      
      
      progress: NaN,
    });

    try {
      filePath = yield this._downloadApk(aMessage.app.manifestURL);
    } catch(ex) {
      aMessage.error = ex;
      aMessageManager.sendAsyncMessage("Webapps:Install:Return:KO", aMessage);
      debug("error downloading APK: " + ex);
      return;
    } finally {
      downloadingNotification.cancel();
    }

    Messaging.sendRequestForResult({
      type: "Webapps:InstallApk",
      filePath: filePath,
      data: aMessage,
    }).catch(function (error) {
      aMessage.error = error;
      aMessageManager.sendAsyncMessage("Webapps:Install:Return:KO", aMessage);
      debug("error downloading APK: " + error);
    });
  }).bind(this)); },

  _downloadApk: function(aManifestUrl) {
    debug("_downloadApk for " + aManifestUrl);
    let deferred = Promise.defer();

    
    const GENERATOR_URL_PREF = "browser.webapps.apkFactoryUrl";
    const GENERATOR_URL_BASE = Services.prefs.getCharPref(GENERATOR_URL_PREF);
    let generatorUrl = NetUtil.newURI(GENERATOR_URL_BASE).QueryInterface(Ci.nsIURL);

    
    let params = {
      manifestUrl: aManifestUrl,
    };
    generatorUrl.query =
      [p + "=" + encodeURIComponent(params[p]) for (p in params)].join("&");
    debug("downloading APK from " + generatorUrl.spec);

    let file = Cc["@mozilla.org/download-manager;1"].
               getService(Ci.nsIDownloadManager).
               defaultDownloadsDirectory.
               clone();
    file.append(aManifestUrl.replace(/[^a-zA-Z0-9]/gi, "") + ".apk");
    file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
    debug("downloading APK to " + file.path);

    let worker = new ChromeWorker("resource://gre/modules/WebappManagerWorker.js");
    worker.onmessage = function(event) {
      let { type, message } = event.data;

      worker.terminate();

      if (type == "success") {
        deferred.resolve(file.path);
      } else { 
        debug("error downloading APK: " + message);
        deferred.reject(message);
      }
    }


    
    worker.postMessage({ url: generatorUrl.spec, path: file.path });

    return deferred.promise;
  },

  _deleteAppcachePath: function(aManifest) {
    
    
    
    
    
    
    if ("appcache_path" in aManifest) {
      debug("deleting appcache_path from manifest: " + aManifest.appcache_path);
      delete aManifest.appcache_path;
    }
  },

  askInstall: function(aData) {
    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
    file.initWithPath(aData.profilePath);

    this._deleteAppcachePath(aData.app.manifest);

    DOMApplicationRegistry.registryReady.then(() => {
      DOMApplicationRegistry.confirmInstall(aData, file, (function(aApp, aManifest) {
        this._postInstall(aData.profilePath, aManifest, aData.app.origin,
                          aData.app.apkPackageName, aData.app.manifestURL);
      }).bind(this));
    });
  },

  _postInstall: function(aProfilePath, aNewManifest, aOrigin, aApkPackageName, aManifestURL) {
    
    Messaging.sendRequest({
      type: "Webapps:Postinstall",
      apkPackageName: aApkPackageName,
      origin: aOrigin,
    });
  },

  askUninstall: function(aData) {
    
    
    DOMApplicationRegistry.denyUninstall(aData, "NOT_SUPPORTED");
  },

  launch: function({ apkPackageName }) {
    debug("launch: " + apkPackageName);

    Messaging.sendRequest({
      type: "Webapps:Launch",
      packageName: apkPackageName,
    });
  },

  uninstall: Task.async(function*(aData, aMessageManager) {
    debug("uninstall: " + aData.manifestURL);

    yield DOMApplicationRegistry.registryReady;

    if (this._testing) {
      
      DOMApplicationRegistry.doUninstall(aData, aMessageManager);
      return;
    }

    let app = DOMApplicationRegistry.getAppByManifestURL(aData.manifestURL);
    if (!app) {
      throw new Error("app not found in registry");
    }

    
    
    let apkVersions = yield this._getAPKVersions([ app.apkPackageName ]);
    if (app.apkPackageName in apkVersions) {
      debug("APK is installed; requesting uninstallation");
      Messaging.sendRequest({
        type: "Webapps:UninstallApk",
        apkPackageName: app.apkPackageName,
      });

      
      
      
      

      
      
      
      
      
      
      
    } else {
      
      
      
      debug("APK not installed; proceeding directly to removal from registry");
      DOMApplicationRegistry.doUninstall(aData, aMessageManager);
    }

  }),

  inGuestSession: function() {
    return Services.wm.getMostRecentWindow("navigator:browser").BrowserApp.isGuest;
  },

  autoInstall: function(aData) {
    debug("autoInstall " + aData.manifestURL);

    
    
    
    
    for (let [ , app] in Iterator(DOMApplicationRegistry.webapps)) {
      if (app.manifestURL == aData.manifestURL) {
        return this._autoUpdate(aData, app);
      }
    }

    let mm = {
      sendAsyncMessage: function (aMessageName, aData) {
        
        debug("sendAsyncMessage " + aMessageName + ": " + JSON.stringify(aData));
      }
    };

    let origin = Services.io.newURI(aData.manifestURL, null, null).prePath;

    let message = aData.request || {
      app: {
        origin: origin,
        receipts: [],
      }
    };

    if (aData.updateManifest) {
      if (aData.zipFilePath) {
        aData.updateManifest.package_path = aData.zipFilePath;
      }
      message.app.updateManifest = aData.updateManifest;
    }

    
    
    
    message.app.manifestURL = aData.manifestURL;
    message.app.manifest = aData.manifest;
    message.app.apkPackageName = aData.apkPackageName;
    message.profilePath = aData.profilePath;
    message.mm = mm;
    message.apkInstall = true;

    DOMApplicationRegistry.registryReady.then(() => {
      switch (aData.type) { 
        case "hosted":
          DOMApplicationRegistry.doInstall(message, mm);
          break;

        case "packaged":
          message.isPackage = true;
          DOMApplicationRegistry.doInstallPackage(message, mm);
          break;
      }
    });
  },

  _autoUpdate: function(aData, aOldApp) { return Task.spawn((function*() {
    debug("_autoUpdate app of type " + aData.type);

    if (aOldApp.apkPackageName != aData.apkPackageName) {
      
      
      debug("update apkPackageName from " + aOldApp.apkPackageName + " to " + aData.apkPackageName);
      aOldApp.apkPackageName = aData.apkPackageName;
    }

    if (aData.type == "hosted") {
      this._deleteAppcachePath(aData.manifest);
      let oldManifest = yield DOMApplicationRegistry.getManifestFor(aData.manifestURL);
      yield DOMApplicationRegistry.updateHostedApp(aData, aOldApp.id, aOldApp, oldManifest, aData.manifest);
    } else {
      yield this._autoUpdatePackagedApp(aData, aOldApp);
    }

    this._postInstall(aData.profilePath, aData.manifest, aOldApp.origin, aOldApp.apkPackageName, aOldApp.manifestURL);
  }).bind(this)); },

  _autoUpdatePackagedApp: Task.async(function*(aData, aOldApp) {
    debug("_autoUpdatePackagedApp: " + aData.manifestURL);

    if (aData.updateManifest && aData.zipFilePath) {
      aData.updateManifest.package_path = aData.zipFilePath;
    }

    
    
    
    yield DOMApplicationRegistry.updatePackagedApp(aData, aOldApp.id, aOldApp, aData.updateManifest);

    try {
      yield DOMApplicationRegistry.startDownload(aData.manifestURL);
    } catch (ex if ex.message == "PACKAGE_UNCHANGED") {
      debug("package unchanged");
      
      return;
    }

    yield DOMApplicationRegistry.applyDownload(aData.manifestURL);
  }),

  _checkingForUpdates: false,

  checkForUpdates: function(userInitiated) { return Task.spawn((function*() {
    debug("checkForUpdates");

    
    
    
    if (this._checkingForUpdates) {
      debug("already checking for updates");
      return;
    }
    this._checkingForUpdates = true;

    try {
      let installedApps = yield this._getInstalledApps();
      if (installedApps.length === 0) {
        return;
      }

      
      let apkNameToVersion = yield this._getAPKVersions(installedApps.map(app =>
        app.apkPackageName).filter(apkPackageName => !!apkPackageName)
      );

      
      
      
      
      
      let manifestUrlToApkVersion = {};
      let manifestUrlToApp = {};
      for (let app of installedApps) {
        manifestUrlToApkVersion[app.manifestURL] = apkNameToVersion[app.apkPackageName] || 0;
        manifestUrlToApp[app.manifestURL] = app;
      }

      let outdatedApps = yield this._getOutdatedApps(manifestUrlToApkVersion, userInitiated);

      if (outdatedApps.length === 0) {
        
        if (userInitiated) {
          this._notify({
            title: Strings.GetStringFromName("noUpdatesTitle"),
            message: Strings.GetStringFromName("noUpdatesMessage"),
            icon: "drawable://alert_app",
          });
        }
        return;
      }

      let usingLan = function() {
        let network = Cc["@mozilla.org/network/network-link-service;1"].getService(Ci.nsINetworkLinkService);
        return (network.linkType == network.LINK_TYPE_WIFI || network.linkType == network.LINK_TYPE_ETHERNET);
      };

      let updateAllowed = function() {
        let autoUpdatePref = Services.prefs.getCharPref("app.update.autodownload");

        return (autoUpdatePref == "enabled") || (autoUpdatePref == "wifi" && usingLan());
      };

      if (updateAllowed()) {
        yield this._updateApks([manifestUrlToApp[url] for (url of outdatedApps)]);
      } else {
        let names = [manifestUrlToApp[url].name for (url of outdatedApps)].join(", ");
        let accepted = yield this._notify({
          title: PluralForm.get(outdatedApps.length, Strings.GetStringFromName("retrieveUpdateTitle")).
                 replace("#1", outdatedApps.length),
          message: getFormattedPluralForm("retrieveUpdateMessage", [names], outdatedApps.length),
          icon: "drawable://alert_app",
        }).dismissed;

        if (accepted) {
          yield this._updateApks([manifestUrlToApp[url] for (url of outdatedApps)]);
        }
      }
    }
    
    
    finally {
      
      
      this._checkingForUpdates = false;
    }
  }).bind(this)); },

  _getAPKVersions: function(packageNames) {
    return Messaging.sendRequestForResult({
      type: "Webapps:GetApkVersions",
      packageNames: packageNames 
    }).then(data => data.versions);
  },

  _getInstalledApps: function() {
    let deferred = Promise.defer();
    DOMApplicationRegistry.getAll(apps => deferred.resolve(apps));
    return deferred.promise;
  },

  _getOutdatedApps: function(installedApps, userInitiated) {
    let deferred = Promise.defer();

    let data = JSON.stringify({ installed: installedApps });

    let notification;
    if (userInitiated) {
      notification = this._notify({
        title: Strings.GetStringFromName("checkingForUpdatesTitle"),
        message: Strings.GetStringFromName("checkingForUpdatesMessage"),
        icon: "drawable://alert_app_animation",
        progress: NaN,
      });
    }

    let request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                  createInstance(Ci.nsIXMLHttpRequest).
                  QueryInterface(Ci.nsIXMLHttpRequestEventTarget);
    request.mozBackgroundRequest = true;
    request.open("POST", Services.prefs.getCharPref(UPDATE_URL_PREF), true);
    request.channel.loadFlags = Ci.nsIChannel.LOAD_ANONYMOUS |
                                Ci.nsIChannel.LOAD_BYPASS_CACHE |
                                Ci.nsIChannel.INHIBIT_CACHING;
    request.onload = function() {
      if (userInitiated) {
        notification.cancel();
      }
      deferred.resolve(JSON.parse(this.response).outdated);
    };
    request.onerror = function() {
      if (userInitiated) {
        notification.cancel();
      }
      deferred.reject(this.status || this.statusText);
    };
    request.setRequestHeader("Content-Type", "application/json");
    request.setRequestHeader("Content-Length", data.length);

    request.send(data);

    return deferred.promise;
  },

  _updateApks: function(aApps) { return Task.spawn((function*() {
    
    let downloadingNames = [app.name for (app of aApps)].join(", ");
    let notification = this._notify({
      title: PluralForm.get(aApps.length, Strings.GetStringFromName("retrievingUpdateTitle")).
             replace("#1", aApps.length),
      message: getFormattedPluralForm("retrievingUpdateMessage", [downloadingNames], aApps.length),
      icon: "drawable://alert_download_animation",
      
      
      progress: NaN,
    });

    
    
    
    
    let downloadedApks = [];
    let downloadFailedApps = [];
    for (let app of aApps) {
      try {
        let filePath = yield this._downloadApk(app.manifestURL);
        downloadedApks.push({ app: app, filePath: filePath });
      } catch(ex) {
        downloadFailedApps.push(app);
      }
    }

    notification.cancel();

    
    
    
    if (downloadFailedApps.length > 0) {
      let downloadFailedNames = [app.name for (app of downloadFailedApps)].join(", ");
      this._notify({
        title: PluralForm.get(downloadFailedApps.length, Strings.GetStringFromName("retrievalFailedTitle")).
               replace("#1", downloadFailedApps.length),
        message: getFormattedPluralForm("retrievalFailedMessage", [downloadFailedNames], downloadFailedApps.length),
        icon: "drawable://alert_app",
      });
    }

    
    if (downloadedApks.length === 0) {
      return;
    }

    
    
    let downloadedNames = [apk.app.name for (apk of downloadedApks)].join(", ");
    let accepted = yield this._notify({
      title: PluralForm.get(downloadedApks.length, Strings.GetStringFromName("installUpdateTitle")).
             replace("#1", downloadedApks.length),
      message: getFormattedPluralForm("installUpdateMessage2", [downloadedNames], downloadedApks.length),
      icon: "drawable://alert_app",
    }).dismissed;

    if (accepted) {
      
      for (let apk of downloadedApks) {
        let msg = {
          app: apk.app,
          
          from: apk.app.installOrigin,
        };
        Messaging.sendRequestForResult({
          type: "Webapps:InstallApk",
          filePath: apk.filePath,
          data: msg,
        }).catch((error) => {
          
          
          debug("APK install failed : " + error);
        });
      }
    } else {
      
      for (let apk of downloadedApks) {
        try {
          yield OS.file.remove(apk.filePath);
        } catch(ex) {
          debug("error removing " + apk.filePath + " for cancelled update: " + ex);
        }
      }
    }

  }).bind(this)); },

  _notify: function(aOptions) {
    dump("_notify: " + aOptions.title);

    
    
    let dismissed = Promise.defer();

    
    
    let id = Notifications.create({
      title: aOptions.title,
      message: aOptions.message,
      icon: aOptions.icon,
      progress: aOptions.progress,
      onClick: function(aId, aCookie) {
        dismissed.resolve(true);
      },
      onCancel: function(aId, aCookie) {
        dismissed.resolve(false);
      },
    });

    
    
    
    
    return {
      dismissed: dismissed.promise,
      cancel: function() {
        Notifications.cancel(id);
      },
    };
  },

  autoUninstall: function(aData) {
    DOMApplicationRegistry.registryReady.then(() => {
      for (let id in DOMApplicationRegistry.webapps) {
        let app = DOMApplicationRegistry.webapps[id];
        if (aData.apkPackageNames.indexOf(app.apkPackageName) > -1) {
          debug("attempting to uninstall " + app.name);
          DOMApplicationRegistry.uninstall(app.manifestURL).then(
            function() {
              debug("success uninstalling " + app.name);
            },
            function(error) {
              debug("error uninstalling " + app.name + ": " + error);
            }
          );
        }
      }
    });
  },
};
