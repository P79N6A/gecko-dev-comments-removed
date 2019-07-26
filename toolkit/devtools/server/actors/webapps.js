



"use strict";

let Cu = Components.utils;
let Cc = Components.classes;
let Ci = Components.interfaces;
let CC = Components.Constructor;

Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/XPCOMUtils.jsm");

let {devtools} = Cu.import("resource://gre/modules/devtools/Loader.jsm", {});

XPCOMUtils.defineLazyGetter(this, "NetworkMonitorManager", () => {
  return devtools.require("devtools/toolkit/webconsole/network-monitor")
         .NetworkMonitorManager;
});

let promise;

function debug(aMsg) {
  




}

function PackageUploadActor(aPath, aFile) {
  this._path = aPath;
  this._file = aFile;
  this.size = 0;
}

PackageUploadActor.prototype = {
  actorPrefix: "packageUploadActor",

  




  getFilePath: function () {
    return this._path;
  },

  



  chunk: function (aRequest) {
    let chunk = aRequest.chunk;
    if (!chunk || chunk.length <= 0) {
      return {error: "parameterError",
              message: "Missing or invalid chunk argument"};
    }
    
    
    let data = new Uint8Array(chunk.length);
    for (let i = 0, l = chunk.length; i < l ; i++) {
      data[i] = chunk.charCodeAt(i);
    }
    return this._file.write(data)
               .then((written) => {
                 this.size += written;
                 return {
                   written: written,
                   size: this.size
                 };
               });
  },

  





  done: function (aRequest) {
    this._file.close();
    return {};
  },

  



  remove: function (aRequest) {
    this._cleanupFile();
    return {};
  },

  _cleanupFile: function () {
    try {
      this._file.close();
    } catch(e) {}
    try {
      OS.File.remove(this._path);
    } catch(e) {}
  }
};




PackageUploadActor.prototype.requestTypes = {
  "chunk": PackageUploadActor.prototype.chunk,
  "done": PackageUploadActor.prototype.done,
  "remove": PackageUploadActor.prototype.remove
};





function WebappsActor(aConnection) {
  debug("init");
  
  

  Cu.import("resource://gre/modules/Webapps.jsm");
  Cu.import("resource://gre/modules/AppsUtils.jsm");
  Cu.import("resource://gre/modules/FileUtils.jsm");

  
  
  this._appActorsMap = new Map();

  this.conn = aConnection;
  this._uploads = [];
  this._actorPool = new ActorPool(this.conn);
  this.conn.addActorPool(this._actorPool);
}

WebappsActor.prototype = {
  actorPrefix: "webapps",

  disconnect: function () {
    
    for (let upload of this._uploads) {
      upload.remove();
    }
    this._uploads = null;

    this.conn.removeActorPool(this._actorPool);
    this._actorPool = null;
    this.conn = null;
  },

  _registerApp: function wa_actorRegisterApp(aDeferred, aApp, aId, aDir) {
    debug("registerApp");
    let reg = DOMApplicationRegistry;
    let self = this;

    
    if (aId in reg._manifestCache) {
      delete reg._manifestCache[aId];
    }

    aApp.installTime = Date.now();
    aApp.installState = "installed";
    aApp.removable = true;
    aApp.id = aId;
    aApp.basePath = reg.getWebAppsBasePath();
    aApp.localId = (aId in reg.webapps) ? reg.webapps[aId].localId
                                        : reg._nextLocalId();

    reg.webapps[aId] = aApp;
    reg.updatePermissionsForApp(aId);

    reg._readManifests([{ id: aId }]).then((aResult) => {
      let manifest = aResult[0].manifest;
      aApp.name = manifest.name;
      reg.updateAppHandlers(null, manifest, aApp);

      reg._saveApps().then(() => {
        aApp.manifest = manifest;

        
        
        
        reg.broadcastMessage("Webapps:UpdateState", {
          app: aApp,
          manifest: manifest,
          manifestURL: aApp.manifestURL
        });
        reg.broadcastMessage("Webapps:FireEvent", {
          eventType: ["downloadsuccess", "downloadapplied"],
          manifestURL: aApp.manifestURL
        });
        reg.broadcastMessage("Webapps:AddApp", { id: aId, app: aApp });
        reg.broadcastMessage("Webapps:Install:Return:OK", {
          app: aApp,
          oid: "foo",
          requestID: "bar"
        });

        Services.obs.notifyObservers(null, "webapps-installed",
          JSON.stringify({ manifestURL: aApp.manifestURL }));

        delete aApp.manifest;
        aDeferred.resolve({ appId: aId, path: aDir.path });

        
        if (!aApp.origin.startsWith("app://")) {
          reg.startOfflineCacheDownload(new ManifestHelper(manifest, aApp.origin));
        }
      });
      
      if (aDir.exists())
        aDir.remove(true);
    });
  },

  _sendError: function wa_actorSendError(aDeferred, aMsg, aId) {
    debug("Sending error: " + aMsg);
    aDeferred.resolve({
      error: "installationFailed",
      message: aMsg,
      appId: aId
    });
  },

  _getAppType: function wa_actorGetAppType(aType) {
    let type = Ci.nsIPrincipal.APP_STATUS_INSTALLED;

    if (aType) {
      type = aType == "privileged" ? Ci.nsIPrincipal.APP_STATUS_PRIVILEGED
           : aType == "certified" ? Ci.nsIPrincipal.APP_STATUS_CERTIFIED
           : Ci.nsIPrincipal.APP_STATUS_INSTALLED;
    }

    return type;
  },

  uploadPackage: function () {
    debug("uploadPackage\n");
    let tmpDir = FileUtils.getDir("TmpD", ["file-upload"], true, false);
    if (!tmpDir.exists() || !tmpDir.isDirectory()) {
      return {error: "fileAccessError",
              message: "Unable to create temporary folder"};
    }
    let tmpFile = tmpDir;
    tmpFile.append("package.zip");
    tmpFile.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("0666", 8));
    if (!tmpFile.exists() || !tmpDir.isFile()) {
      return {error: "fileAccessError",
              message: "Unable to create temporary file"};
    }

    return OS.File.open(tmpFile.path, { write: true, truncate: true })
             .then((file) => {
                let actor = new PackageUploadActor(tmpFile.path, file);
                this._actorPool.addActor(actor);
                this._uploads.push(actor);
                return { actor: actor.actorID };
             });
  },

  installHostedApp: function wa_actorInstallHosted(aDir, aId, aReceipts,
                                                   aManifest, aMetadata) {
    debug("installHostedApp");
    let self = this;
    let deferred = promise.defer();

    function readManifest() {
      if (aManifest) {
        return promise.resolve(aManifest);
      } else {
        let manFile = OS.Path.join(aDir.path, "manifest.webapp");
        return AppsUtils.loadJSONAsync(manFile);
      }
    }
    function checkSideloading(aManifest) {
      return self._getAppType(aManifest.type);
    }
    function writeManifest(aAppType) {
      
      
      let installDir = DOMApplicationRegistry._getAppDir(aId);
      if (aManifest) {
        let manFile = OS.Path.join(installDir.path, "manifest.webapp");
        return DOMApplicationRegistry._writeFile(manFile, JSON.stringify(aManifest)).then(() => {
          return aAppType;
        });
      } else {
        let manFile = aDir.clone();
        manFile.append("manifest.webapp");
        manFile.moveTo(installDir, "manifest.webapp");
      }
      return null;
    }
    function readMetadata(aAppType) {
      if (aMetadata) {
        return { metadata: aMetadata, appType: aAppType };
      }
      
      let metaFile = OS.Path.join(aDir.path, "metadata.json");
      return AppsUtils.loadJSONAsync(metaFile).then((aMetadata) => {
        if (!aMetadata) {
          throw("Error parsing metadata.json.");
        }
        if (!aMetadata.origin) {
          throw("Missing 'origin' property in metadata.json");
        }
        return { metadata: aMetadata, appType: aAppType };
      });
    }
    let runnable = {
      run: function run() {
        try {
          readManifest().
            then(writeManifest).
            then(checkSideloading).
            then(readMetadata).
            then(function ({ metadata, appType }) {
              let origin = metadata.origin;
              let manifestURL = metadata.manifestURL ||
                                origin + "/manifest.webapp";
              
              let app = {
                origin: origin,
                installOrigin: metadata.installOrigin || origin,
                manifestURL: manifestURL,
                appStatus: appType,
                receipts: aReceipts,
              };

              self._registerApp(deferred, app, aId, aDir);
            }, function (error) {
              self._sendError(deferred, error, aId);
            });
        } catch(e) {
          
          self._sendError(deferred, e.toString(), aId);
        }
      }
    }

    Services.tm.currentThread.dispatch(runnable,
                                       Ci.nsIThread.DISPATCH_NORMAL);
    return deferred.promise;
  },

  installPackagedApp: function wa_actorInstallPackaged(aDir, aId, aReceipts) {
    debug("installPackagedApp");
    let self = this;
    let deferred = promise.defer();

    let runnable = {
      run: function run() {
        try {
          
          let zipFile = aDir.clone();
          zipFile.append("application.zip");
          let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"]
                            .createInstance(Ci.nsIZipReader);
          zipReader.open(zipFile);

          
          let istream = zipReader.getInputStream("manifest.webapp");
          let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                            .createInstance(Ci.nsIScriptableUnicodeConverter);
          converter.charset = "UTF-8";
          let jsonString = converter.ConvertToUnicode(
            NetUtil.readInputStreamToString(istream, istream.available())
          );

          let manifest;
          try {
            manifest = JSON.parse(jsonString);
          } catch(e) {
            self._sendError(deferred, "Error Parsing manifest.webapp: " + e, aId);
          }

          let appType = self._getAppType(manifest.type);

          
          
          let id = aId;
          if (appType >= Ci.nsIPrincipal.APP_STATUS_PRIVILEGED &&
              manifest.origin !== undefined) {
            let uri;
            try {
              uri = Services.io.newURI(manifest.origin, null, null);
            } catch(e) {
              self._sendError(deferred, "Invalid origin in webapp's manifest", aId);
            }

            if (uri.scheme != "app") {
              self._sendError(deferred, "Invalid origin in webapp's manifest", aId);
            }
            id = uri.prePath.substring(6);
          }

          
          
          
          let installDir = DOMApplicationRegistry._getAppDir(id);
          let manFile = installDir.clone();
          manFile.append("manifest.webapp");
          zipReader.extract("manifest.webapp", manFile);
          zipReader.close();
          zipFile.moveTo(installDir, "application.zip");

          let origin = "app://" + id;
          let manifestURL = origin + "/manifest.webapp";

          
          
          
          let jar = installDir.clone();
          jar.append("application.zip");
          Services.obs.notifyObservers(jar, "flush-cache-entry", null);

          
          
          
          
          let FlushFrameScript = function (path) {
            let jar = Components.classes["@mozilla.org/file/local;1"]
                                .createInstance(Components.interfaces.nsILocalFile);
            jar.initWithPath(path);
            let obs = Components.classes["@mozilla.org/observer-service;1"]
                                .getService(Components.interfaces.nsIObserverService);
            obs.notifyObservers(jar, "flush-cache-entry", null);
          };
          for each (let frame in self._appFrames()) {
            if (frame.getAttribute("mozapp") == manifestURL) {
              let mm = frame.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader.messageManager;
              mm.loadFrameScript("data:," +
                encodeURIComponent("(" + FlushFrameScript.toString() + ")" +
                                   "('" + jar.path + "')"), false);
            }
          }

          
          let app = {
            origin: origin,
            installOrigin: origin,
            manifestURL: manifestURL,
            appStatus: appType,
            receipts: aReceipts,
          }

          self._registerApp(deferred, app, id, aDir);
        } catch(e) {
          
          self._sendError(deferred, e.toString(), aId);
        }
      }
    }

    Services.tm.currentThread.dispatch(runnable,
                                       Ci.nsIThread.DISPATCH_NORMAL);
    return deferred.promise;
  },

  





  install: function wa_actorInstall(aRequest) {
    debug("install");

    let appId = aRequest.appId;
    let reg = DOMApplicationRegistry;
    if (!appId) {
      appId = reg.makeAppId();
    }

    
    if (appId in reg.webapps && reg.webapps[appId].removable === false) {
      return { error: "badParameterType",
               message: "The application " + appId + " can't be overriden."
             }
    }

    let appDir = FileUtils.getDir("TmpD", ["b2g", appId], false, false);

    if (aRequest.upload) {
      
      appDir = FileUtils.getDir("TmpD", ["b2g", appId], true, false);
      let actor = this.conn.getActor(aRequest.upload);
      if (!actor) {
        return { error: "badParameter",
                 message: "Unable to find upload actor '" + aRequest.upload
                          + "'" };
      }
      let appFile = FileUtils.File(actor.getFilePath());
      if (!appFile.exists()) {
        return { error: "badParameter",
                 message: "The uploaded file doesn't exist on device" };
      }
      appFile.moveTo(appDir, "application.zip");
    } else if ((!appDir || !appDir.exists()) &&
               !aRequest.manifest && !aRequest.metadata) {
      return { error: "badParameterType",
               message: "missing directory " + appDir.path
             };
    }

    let testFile = appDir.clone();
    testFile.append("application.zip");

    let receipts = (aRequest.receipts && Array.isArray(aRequest.receipts))
                    ? aRequest.receipts
                    : [];

    if (testFile.exists()) {
      return this.installPackagedApp(appDir, appId, receipts);
    }

    let manifest, metadata;
    let missing =
      ["manifest.webapp", "metadata.json"]
      .some(function(aName) {
        testFile = appDir.clone();
        testFile.append(aName);
        return !testFile.exists();
      });
    if (missing) {
      if (aRequest.manifest && aRequest.metadata &&
          aRequest.metadata.origin) {
        manifest = aRequest.manifest;
        metadata = aRequest.metadata;
      } else {
        try {
          appDir.remove(true);
        } catch(e) {}
        return { error: "badParameterType",
                 message: "hosted app file and manifest/metadata fields " +
                          "are missing"
        };
      }
    }

    return this.installHostedApp(appDir, appId, receipts, manifest, metadata);
  },

  getAll: function wa_actorGetAll(aRequest) {
    debug("getAll");

    let deferred = promise.defer();
    let reg = DOMApplicationRegistry;
    reg.getAll(apps => {
      deferred.resolve({ apps: this._filterAllowedApps(apps) });
    });

    return deferred.promise;
  },

  getApp: function wa_actorGetApp(aRequest) {
    debug("getApp");

    let manifestURL = aRequest.manifestURL;
    if (!manifestURL) {
      return { error: "missingParameter",
               message: "missing parameter manifestURL" };
    }

    let reg = DOMApplicationRegistry;
    let app = reg.getAppByManifestURL(manifestURL);
    if (!app) {
      return { error: "appNotFound" };
    }

    return this._isAppAllowedForURL(app.manifestURL).then(allowed => {
      if (!allowed) {
        return { error: "forbidden" };
      }
      return reg.getManifestFor(manifestURL).then(function (manifest) {
        app.manifest = manifest;
        return { app: app };
      });
    });
  },

  _areCertifiedAppsAllowed: function wa__areCertifiedAppsAllowed() {
    let pref = "devtools.debugger.forbid-certified-apps";
    return !Services.prefs.getBoolPref(pref);
  },

  _isAppAllowedForManifest: function wa__isAppAllowedForManifest(aManifest) {
    if (this._areCertifiedAppsAllowed()) {
      return true;
    }
    let type = this._getAppType(aManifest.type);
    return type !== Ci.nsIPrincipal.APP_STATUS_CERTIFIED;
  },

  _filterAllowedApps: function wa__filterAllowedApps(aApps) {
    return aApps.filter(app => this._isAppAllowedForManifest(app.manifest));
  },

  _isAppAllowedForURL: function wa__isAppAllowedForURL(aManifestURL) {
    return this._findManifestByURL(aManifestURL).then(manifest => {
      return this._isAppAllowedForManifest(manifest);
    });
  },

  uninstall: function wa_actorUninstall(aRequest) {
    debug("uninstall");

    let manifestURL = aRequest.manifestURL;
    if (!manifestURL) {
      return { error: "missingParameter",
               message: "missing parameter manifestURL" };
    }

    let deferred = promise.defer();
    let reg = DOMApplicationRegistry;
    reg.uninstall(
      manifestURL,
      function onsuccess() {
        deferred.resolve({});
      },
      function onfailure(reason) {
        deferred.resolve({ error: reason });
      }
    );

    return deferred.promise;
  },

  _findManifestByURL: function wa__findManifestByURL(aManifestURL) {
    let deferred = promise.defer();

    let reg = DOMApplicationRegistry;
    let id = reg._appIdForManifestURL(aManifestURL);

    reg._readManifests([{ id: id }]).then((aResults) => {
      deferred.resolve(aResults[0].manifest);
    });

    return deferred.promise;
  },

  getIconAsDataURL: function (aRequest) {
    debug("getIconAsDataURL");

    let manifestURL = aRequest.manifestURL;
    if (!manifestURL) {
      return { error: "missingParameter",
               message: "missing parameter manifestURL" };
    }

    let reg = DOMApplicationRegistry;
    let app = reg.getAppByManifestURL(manifestURL);
    if (!app) {
      return { error: "wrongParameter",
               message: "No application for " + manifestURL };
    }

    let deferred = promise.defer();

    this._findManifestByURL(manifestURL).then(jsonManifest => {
      let manifest = new ManifestHelper(jsonManifest, app.origin);
      let iconURL = manifest.iconURLForSize(aRequest.size || 128);
      if (!iconURL) {
        deferred.resolve({
          error: "noIcon",
          message: "This app has no icon"
        });
        return;
      }

      
      
      
      let req = Cc['@mozilla.org/xmlextras/xmlhttprequest;1']
                  .createInstance(Ci.nsIXMLHttpRequest);
      req.open("GET", iconURL, false);
      req.responseType = "blob";

      try {
        req.send(null);
      } catch(e) {
        deferred.resolve({
          error: "noIcon",
          message: "The icon file '" + iconURL + "' doesn't exist"
        });
        return;
      }

      
      let reader = Cc["@mozilla.org/files/filereader;1"]
                     .createInstance(Ci.nsIDOMFileReader);
      reader.onload = function () {
        deferred.resolve({
          url: reader.result
        });
      };
      reader.onerror = function () {
        deferred.resolve({
          error: reader.error.name,
          message: String(reader.error)
        });
      };
      reader.readAsDataURL(req.response);
    });

    return deferred.promise;
  },

  launch: function wa_actorLaunch(aRequest) {
    debug("launch");

    let manifestURL = aRequest.manifestURL;
    if (!manifestURL) {
      return { error: "missingParameter",
               message: "missing parameter manifestURL" };
    }

    let deferred = promise.defer();

    DOMApplicationRegistry.launch(
      aRequest.manifestURL,
      aRequest.startPoint || "",
      Date.now(),
      function onsuccess() {
        deferred.resolve({});
      },
      function onfailure(reason) {
        deferred.resolve({ error: reason });
      });

    return deferred.promise;
  },

  close: function wa_actorLaunch(aRequest) {
    debug("close");

    let manifestURL = aRequest.manifestURL;
    if (!manifestURL) {
      return { error: "missingParameter",
               message: "missing parameter manifestURL" };
    }

    let reg = DOMApplicationRegistry;
    let app = reg.getAppByManifestURL(manifestURL);
    if (!app) {
      return { error: "missingParameter",
               message: "No application for " + manifestURL };
    }

    reg.close(app);

    return {};
  },

  _appFrames: function () {
    
    if (Services.appinfo.ID != "{3c2e2abc-06d4-11e1-ac3b-374f68613e61}") {
      return;
    }
    
    let chromeWindow = Services.wm.getMostRecentWindow('navigator:browser');
    let systemAppFrame = chromeWindow.shell.contentBrowser;
    yield systemAppFrame;

    
    
    
    
    let frames = systemAppFrame.contentDocument.querySelectorAll("iframe[mozapp]");
    for (let i = 0; i < frames.length; i++) {
      yield frames[i];
    }
  },

  listRunningApps: function (aRequest) {
    debug("listRunningApps\n");

    let appPromises = [];
    let apps = [];

    for each (let frame in this._appFrames()) {
      let manifestURL = frame.getAttribute("mozapp");

      appPromises.push(this._isAppAllowedForURL(manifestURL).then(allowed => {
        if (allowed) {
          apps.push(manifestURL);
        }
      }));
    }

    return promise.all(appPromises).then(() => {
      return { apps: apps };
    });
  },

  getAppActor: function ({ manifestURL }) {
    debug("getAppActor\n");

    let appFrame = null;
    for each (let frame in this._appFrames()) {
      if (frame.getAttribute("mozapp") == manifestURL) {
        appFrame = frame;
        break;
      }
    }

    let notFoundError = {
      error: "appNotFound",
      message: "Unable to find any opened app whose manifest " +
               "is '" + manifestURL + "'"
    };

    if (!appFrame) {
      return notFoundError;
    }

    return this._isAppAllowedForURL(manifestURL).then(allowed => {
      if (!allowed) {
        return notFoundError;
      }

      
      
      let map = this._appActorsMap;
      let mm = appFrame.QueryInterface(Ci.nsIFrameLoaderOwner)
                       .frameLoader
                       .messageManager;
      let actor = map.get(mm);
      let netMonitor = null;
      if (!actor) {
        let onConnect = actor => {
          map.set(mm, actor);
          netMonitor = new NetworkMonitorManager(appFrame);
          return { actor: actor };
        };
        let onDisconnect = mm => {
          map.delete(mm);
          if (netMonitor) {
            netMonitor.destroy();
            netMonitor = null;
          }
        };
        return DebuggerServer.connectToChild(this.conn, mm, onDisconnect)
                             .then(onConnect);
      }

      return { actor: actor };
    });
  },

  watchApps: function () {
    this._openedApps = new Set();
    
    if (Services.appinfo.ID == "{3c2e2abc-06d4-11e1-ac3b-374f68613e61}") {
      let chromeWindow = Services.wm.getMostRecentWindow('navigator:browser');
      let systemAppFrame = chromeWindow.getContentWindow();
      systemAppFrame.addEventListener("appwillopen", this);
      systemAppFrame.addEventListener("appterminated", this);
    }
    Services.obs.addObserver(this, "webapps-installed", false);
    Services.obs.addObserver(this, "webapps-uninstall", false);

    return {};
  },

  unwatchApps: function () {
    this._openedApps = null;
    if (Services.appinfo.ID == "{3c2e2abc-06d4-11e1-ac3b-374f68613e61}") {
      let chromeWindow = Services.wm.getMostRecentWindow('navigator:browser');
      let systemAppFrame = chromeWindow.getContentWindow();
      systemAppFrame.removeEventListener("appwillopen", this);
      systemAppFrame.removeEventListener("appterminated", this);
    }
    Services.obs.removeObserver(this, "webapps-installed", false);
    Services.obs.removeObserver(this, "webapps-uninstall", false);

    return {};
  },

  handleEvent: function (event) {
    let manifestURL;
    switch(event.type) {
      case "appwillopen":
        manifestURL = event.detail.manifestURL;

        
        
        
        if (this._openedApps.has(manifestURL)) {
          return;
        }
        this._openedApps.add(manifestURL);

        this._isAppAllowedForURL(manifestURL).then(allowed => {
          if (allowed) {
            this.conn.send({ from: this.actorID,
                             type: "appOpen",
                             manifestURL: manifestURL
                           });
          }
        });

        break;

      case "appterminated":
        manifestURL = event.detail.manifestURL;
        this._openedApps.delete(manifestURL);

        this._isAppAllowedForURL(manifestURL).then(allowed => {
          if (allowed) {
            this.conn.send({ from: this.actorID,
                             type: "appClose",
                             manifestURL: manifestURL
                           });
          }
        });

        break;
    }
  },

  observe: function (subject, topic, data) {
    let app = JSON.parse(data);
    if (topic == "webapps-installed") {
      this.conn.send({ from: this.actorID,
                       type: "appInstall",
                       manifestURL: app.manifestURL
                     });
    } else if (topic == "webapps-uninstall") {
      this.conn.send({ from: this.actorID,
                       type: "appUninstall",
                       manifestURL: app.manifestURL
                     });
    }
  }
};




WebappsActor.prototype.requestTypes = {
  "install": WebappsActor.prototype.install,
  "uploadPackage": WebappsActor.prototype.uploadPackage,
  "getAll": WebappsActor.prototype.getAll,
  "getApp": WebappsActor.prototype.getApp,
  "launch": WebappsActor.prototype.launch,
  "close": WebappsActor.prototype.close,
  "uninstall": WebappsActor.prototype.uninstall,
  "listRunningApps": WebappsActor.prototype.listRunningApps,
  "getAppActor": WebappsActor.prototype.getAppActor,
  "watchApps": WebappsActor.prototype.watchApps,
  "unwatchApps": WebappsActor.prototype.unwatchApps,
  "getIconAsDataURL": WebappsActor.prototype.getIconAsDataURL
};

DebuggerServer.addGlobalActor(WebappsActor, "webappsActor");
