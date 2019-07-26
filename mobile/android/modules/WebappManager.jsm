



"use strict";

this.EXPORTED_SYMBOLS = ["WebappManager"];

const { classes: Cc, interfaces: Ci, utils: Cu } = Components;

Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/DOMRequestHelper.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/osfile.jsm");

function dump(a) {
  Services.console.logStringMessage("* * WebappManager.jsm: " + a);
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

    this._downloadApk(aMessage, aMessageManager);
  },

  installPackage: function(aMessage, aMessageManager) {
    if (this._testing) {
      
      DOMApplicationRegistry.doInstallPackage(aMessage, aMessageManager);
      return;
    }

    this._downloadApk(aMessage, aMessageManager);
  },

  _downloadApk: function(aMsg, aMessageManager) {
    let manifestUrl = aMsg.app.manifestURL;
    dump("_downloadApk for " + manifestUrl);

    
    const GENERATOR_URL_PREF = "browser.webapps.apkFactoryUrl";
    const GENERATOR_URL_BASE = Services.prefs.getCharPref(GENERATOR_URL_PREF);
    let generatorUrl = NetUtil.newURI(GENERATOR_URL_BASE).QueryInterface(Ci.nsIURL);

    
    let params = {
      manifestUrl: manifestUrl,
    };
    generatorUrl.query =
      [p + "=" + encodeURIComponent(params[p]) for (p in params)].join("&");
    dump("downloading APK from " + generatorUrl.spec);

    let file = Cc["@mozilla.org/download-manager;1"].
               getService(Ci.nsIDownloadManager).
               defaultDownloadsDirectory.
               clone();
    file.append(manifestUrl.replace(/[^a-zA-Z0-9]/gi, "") + ".apk");
    file.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
    dump("downloading APK to " + file.path);

    let worker = new ChromeWorker("resource://gre/modules/WebappManagerWorker.js");
    worker.onmessage = function(event) {
      let { type, message } = event.data;

      worker.terminate();

      if (type == "success") {
        sendMessageToJava({
          type: "WebApps:InstallApk",
          filePath: file.path,
          data: JSON.stringify(aMsg),
        });
      } else { 
        aMsg.error = message;
        aMessageManager.sendAsyncMessage("Webapps:Install:Return:KO", aMsg);
        dump("error downloading APK: " + message);
      }
    }

    
    worker.postMessage({ url: generatorUrl.spec, path: file.path });
  },

  askInstall: function(aData) {
    let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
    file.initWithPath(aData.profilePath);

    
    
    
    
    
    
    if ("appcache_path" in aData.app.manifest) {
      dump("deleting appcache_path from manifest: " + aData.app.manifest.appcache_path);
      delete aData.app.manifest.appcache_path;
    }

    DOMApplicationRegistry.confirmInstall(aData, file, (function(aManifest) {
      let localeManifest = new ManifestHelper(aManifest, aData.app.origin);

      
      sendMessageToJava({
        type: "WebApps:PostInstall",
        apkPackageName: aData.app.apkPackageName,
        origin: aData.app.origin,
      });

      this.writeDefaultPrefs(file, localeManifest);
    }).bind(this));
  },

  launch: function({ manifestURL, origin }) {
    dump("launchWebapp: " + manifestURL);

    sendMessageToJava({
      type: "WebApps:Open",
      manifestURL: manifestURL,
      origin: origin
    });
  },

  uninstall: function(aData) {
    dump("uninstall: " + aData.manifestURL);

    if (this._testing) {
      
      return;
    }

    
  },

  autoInstall: function(aData) {
    let mm = {
      sendAsyncMessage: function (aMessageName, aData) {
        
        dump("sendAsyncMessage " + aMessageName + ": " + JSON.stringify(aData));
      }
    };

    let origin = Services.io.newURI(aData.manifestUrl, null, null).prePath;

    let message = aData.request || {
      app: {
        origin: origin
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

    switch (aData.type) { 
      case "hosted":
        DOMApplicationRegistry.doInstall(message, mm);
        break;

      case "packaged":
        message.isPackage = true;
        DOMApplicationRegistry.doInstallPackage(message, mm);
        break;
    }
  },

  autoUninstall: function(aData) {
    let mm = {
      sendAsyncMessage: function (aMessageName, aData) {
        
        dump("autoUninstall sendAsyncMessage " + aMessageName + ": " + JSON.stringify(aData));
      }
    };
    let installed = {};
    DOMApplicationRegistry.doGetAll(installed, mm);

    for (let app in installed.apps) {
      if (aData.apkPackageNames.indexOf(installed.apps[app].apkPackageName) > -1) {
        let appToRemove = installed.apps[app];
        dump("should remove: " + appToRemove.name);
        DOMApplicationRegistry.uninstall(appToRemove.manifestURL, function() {
          dump(appToRemove.name + " uninstalled");
        }, function() {
          dump(appToRemove.name + " did not uninstall");
        });
      }
    }
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
        dump("Error writing default prefs: " + reason);
      });
    }
  },

  DEFAULT_PREFS_FILENAME: "default-prefs.js",

};
