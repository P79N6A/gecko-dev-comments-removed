



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
Cu.import("resource://gre/modules/PluralForm.jsm");

XPCOMUtils.defineLazyModuleGetter(this, "Notifications", "resource://gre/modules/Notifications.jsm");

XPCOMUtils.defineLazyGetter(this, "Strings", function() {
  return Services.strings.createBundle("chrome://browser/locale/webapp.properties");
});

function log(message) {
  
  
  
  
  
  dump(message);
}

function sendMessageToJava(aMessage) {
  return Services.androidBridge.handleGeckoMessage(JSON.stringify(aMessage));
}

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
    let filePath;

    try {
      filePath = yield this._downloadApk(aMessage.app.manifestURL);
    } catch(ex) {
      aMessage.error = ex;
      aMessageManager.sendAsyncMessage("Webapps:Install:Return:KO", aMessage);
      log("error downloading APK: " + ex);
      return;
    }

    sendMessageToJava({
      type: "Webapps:InstallApk",
      filePath: filePath,
      data: JSON.stringify(aMessage),
    });
  }).bind(this)); },

  _downloadApk: function(aManifestUrl) {
    log("_downloadApk for " + aManifestUrl);
    let deferred = Promise.defer();

    
    const GENERATOR_URL_PREF = "browser.webapps.apkFactoryUrl";
    const GENERATOR_URL_BASE = Services.prefs.getCharPref(GENERATOR_URL_PREF);
    let generatorUrl = NetUtil.newURI(GENERATOR_URL_BASE).QueryInterface(Ci.nsIURL);

    
    let params = {
      manifestUrl: aManifestUrl,
    };
    generatorUrl.query =
      [p + "=" + encodeURIComponent(params[p]) for (p in params)].join("&");
    log("downloading APK from " + generatorUrl.spec);

    let file = Cc["@mozilla.org/download-manager;1"].
               getService(Ci.nsIDownloadManager).
               defaultDownloadsDirectory.
               clone();
    file.append(aManifestUrl.replace(/[^a-zA-Z0-9]/gi, "") + ".apk");
    file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
    log("downloading APK to " + file.path);

    let worker = new ChromeWorker("resource://gre/modules/WebappManagerWorker.js");
    worker.onmessage = function(event) {
      let { type, message } = event.data;

      worker.terminate();

      if (type == "success") {
        deferred.resolve(file.path);
      } else { 
        log("error downloading APK: " + message);
        deferred.reject(message);
      }
    }

    
    worker.postMessage({ url: generatorUrl.spec, path: file.path });

    return deferred.promise;
  },

  askInstall: function(aData) {
    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
    file.initWithPath(aData.profilePath);

    
    
    
    
    
    
    if ("appcache_path" in aData.app.manifest) {
      log("deleting appcache_path from manifest: " + aData.app.manifest.appcache_path);
      delete aData.app.manifest.appcache_path;
    }

    DOMApplicationRegistry.registryReady.then(() => {
      DOMApplicationRegistry.confirmInstall(aData, file, (function(aManifest) {
        let localeManifest = new ManifestHelper(aManifest, aData.app.origin);

        
        sendMessageToJava({
          type: "Webapps:Postinstall",
          apkPackageName: aData.app.apkPackageName,
          origin: aData.app.origin,
        });

        this.writeDefaultPrefs(file, localeManifest);
      }).bind(this));
    });
  },

  launch: function({ manifestURL, origin }) {
    log("launchWebapp: " + manifestURL);

    sendMessageToJava({
      type: "Webapps:Open",
      manifestURL: manifestURL,
      origin: origin
    });
  },

  uninstall: function(aData) {
    log("uninstall: " + aData.manifestURL);

    if (this._testing) {
      
      return;
    }

    
  },

  autoInstall: function(aData) {
    let oldApp = DOMApplicationRegistry.getAppByManifestURL(aData.manifestUrl);
    if (oldApp) {
      
      this._autoUpdate(aData, oldApp);
      return;
    }

    let mm = {
      sendAsyncMessage: function (aMessageName, aData) {
        
        log("sendAsyncMessage " + aMessageName + ": " + JSON.stringify(aData));
      }
    };

    let origin = Services.io.newURI(aData.manifestUrl, null, null).prePath;

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

    
    
    
    message.app.manifestURL = aData.manifestUrl;
    message.app.manifest = aData.manifest;
    message.app.apkPackageName = aData.apkPackageName;
    message.profilePath = aData.profilePath;
    message.autoInstall = true;
    message.mm = mm;

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
    log("_autoUpdate app of type " + aData.type);

    
    
    
    
    
    
    aData.manifestURL = aData.manifestUrl;
    delete aData.manifestUrl;

    if (aData.type == "hosted") {
      let oldManifest = yield DOMApplicationRegistry.getManifestFor(aData.manifestURL);
      DOMApplicationRegistry.updateHostedApp(aData, aOldApp.id, aOldApp, oldManifest, aData.manifest);
    } else {
      DOMApplicationRegistry.updatePackagedApp(aData, aOldApp.id, aOldApp, aData.manifest);
    }
  }).bind(this)); },

  _checkingForUpdates: false,

  checkForUpdates: function(userInitiated) { return Task.spawn((function*() {
    log("checkForUpdates");

    
    
    
    if (this._checkingForUpdates) {
      log("already checking for updates");
      return;
    }
    this._checkingForUpdates = true;

    try {
      let installedApps = yield this._getInstalledApps();
      if (installedApps.length === 0) {
        return;
      }

      
      let apkNameToVersion = JSON.parse(sendMessageToJava({
        type: "Webapps:GetApkVersions",
        packageNames: installedApps.map(app => app.packageName).filter(packageName => !!packageName)
      }));

      
      
      
      
      
      let manifestUrlToApkVersion = {};
      let manifestUrlToApp = {};
      for (let app of installedApps) {
        manifestUrlToApkVersion[app.manifestURL] = apkNameToVersion[app.packageName] || 0;
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

      let names = [manifestUrlToApp[url].name for (url of outdatedApps)].join(", ");
      let accepted = yield this._notify({
        title: PluralForm.get(outdatedApps.length, Strings.GetStringFromName("downloadUpdateTitle")).
               replace("#1", outdatedApps.length),
        message: Strings.formatStringFromName("downloadUpdateMessage", [names], 1),
        icon: "drawable://alert_download",
      }).dismissed;

      if (accepted) {
        yield this._updateApks([manifestUrlToApp[url] for (url of outdatedApps)]);
      }
    }
    
    
    finally {
      
      
      this._checkingForUpdates = false;
    }
  }).bind(this)); },

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
        
        icon: "drawable://alert_app",
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
      notification.cancel();
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
      title: PluralForm.get(aApps.length, Strings.GetStringFromName("downloadingUpdateTitle")).
             replace("#1", aApps.length),
      message: Strings.formatStringFromName("downloadingUpdateMessage", [downloadingNames], 1),
      
      
      
      icon: "drawable://alert_download",
      
      
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
        title: PluralForm.get(downloadFailedApps.length, Strings.GetStringFromName("downloadFailedTitle")).
               replace("#1", downloadFailedApps.length),
        message: Strings.formatStringFromName("downloadFailedMessage", [downloadFailedNames], 1),
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
      message: Strings.formatStringFromName("installUpdateMessage", [downloadedNames], 1),
      icon: "drawable://alert_app",
    }).dismissed;

    if (accepted) {
      
      for (let apk of downloadedApks) {
        let msg = {
          app: apk.app,
          
          from: apk.app.installOrigin,
        };
        sendMessageToJava({
          type: "Webapps:InstallApk",
          filePath: apk.filePath,
          data: JSON.stringify(msg),
        });
      }
    } else {
      
      for (let apk of downloadedApks) {
        try {
          yield OS.file.remove(apk.filePath);
        } catch(ex) {
          log("error removing " + apk.filePath + " for cancelled update: " + ex);
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
          log("attempting to uninstall " + app.name);
          DOMApplicationRegistry.uninstall(
            app.manifestURL,
            function() {
              log("success uninstalling " + app.name);
            },
            function(error) {
              log("error uninstalling " + app.name + ": " + error);
            }
          );
        }
      }
    });
  },

  writeDefaultPrefs: function(aProfile, aManifest) {
      
      let prefs = [];
      if (aManifest.orientation) {
        prefs.push({name:"app.orientation.default", value: aManifest.orientation.join(",") });
      }

      
      let defaultPrefsFile = aProfile.clone();
      defaultPrefsFile.append(this.DEFAULT_PREFS_FILENAME);
      this._writeData(defaultPrefsFile, prefs);
  },

  _writeData: function(aFile, aPrefs) {
    if (aPrefs.length > 0) {
      let array = new TextEncoder().encode(JSON.stringify(aPrefs));
      OS.File.writeAtomic(aFile.path, array, { tmpPath: aFile.path + ".tmp" }).then(null, function onError(reason) {
        log("Error writing default prefs: " + reason);
      });
    }
  },

  DEFAULT_PREFS_FILENAME: "default-prefs.js",

};
