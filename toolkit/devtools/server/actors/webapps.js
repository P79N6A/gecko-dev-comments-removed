



"use strict";

let Cu = Components.utils;
let Cc = Components.classes;
let Ci = Components.interfaces;

function debug(aMsg) {
  




}





function WebappsActor(aConnection) {
  debug("init");
  
  

  Cu.import("resource://gre/modules/Webapps.jsm");
  Cu.import("resource://gre/modules/AppsUtils.jsm");
  Cu.import("resource://gre/modules/FileUtils.jsm");
  Cu.import('resource://gre/modules/Services.jsm');
  Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js");
}

WebappsActor.prototype = {
  actorPrefix: "webapps",

  _registerApp: function wa_actorRegisterApp(aApp, aId, aDir) {
    debug("registerApp");
    let reg = DOMApplicationRegistry;
    let self = this;

    aApp.installTime = Date.now();
    aApp.installState = "installed";
    aApp.removable = true;
    aApp.id = aId;
    aApp.basePath = reg.getWebAppsBasePath();
    aApp.localId = (aId in reg.webapps) ? reg.webapps[aId].localId
                                        : reg._nextLocalId();

    reg.webapps[aId] = aApp;
    reg.updatePermissionsForApp(aId);

    reg._readManifests([{ id: aId }], function(aResult) {
      let manifest = aResult[0].manifest;
      aApp.name = manifest.name;
      if ("_registerSystemMessages" in reg) {
        reg._registerSystemMessages(manifest, aApp);
      }
      if ("_registerActivities" in reg) {
        reg._registerActivities(manifest, aApp, true);
      }
      reg._saveApps(function() {
        aApp.manifest = manifest;
        reg.broadcastMessage("Webapps:AddApp", { id: aId, app: aApp });
        reg.broadcastMessage("Webapps:Install:Return:OK",
                             { app: aApp,
                               oid: "foo",
                               requestID: "bar"
                             });
        delete aApp.manifest;
        self.conn.send({ from: self.actorID,
                         type: "webappsEvent",
                         appId: aId
                       });

        
        if (!aApp.origin.startsWith("app://")) {
          reg.startOfflineCacheDownload(new ManifestHelper(manifest, aApp.origin));
        }
      });
      
      aDir.remove(true);
    });
  },

  _sendError: function wa_actorSendError(aMsg, aId) {
    debug("Sending error: " + aMsg);
    this.conn.send(
      { from: this.actorID,
        type: "webappsEvent",
        appId: aId,
        error: "installationFailed",
        message: aMsg
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

  installHostedApp: function wa_actorInstallHosted(aDir, aId, aReceipts) {
    debug("installHostedApp");
    let self = this;

    let runnable = {
      run: function run() {
        try {
          
          let manFile = aDir.clone();
          manFile.append("manifest.webapp");
          DOMApplicationRegistry._loadJSONAsync(manFile, function(aManifest) {
            if (!aManifest) {
              self._sendError("Error Parsing manifest.webapp", aId);
              return;
            }

            let appType = self._getAppType(aManifest.type);

            
            if (!DOMApplicationRegistry.allowSideloadingCertified &&
                appType == Ci.nsIPrincipal.APP_STATUS_CERTIFIED) {
              self._sendError("Installing certified apps is not allowed.", aId);
              return;
            }

            
            let installDir = DOMApplicationRegistry._getAppDir(aId);
            manFile.moveTo(installDir, "manifest.webapp");

            
            let metaFile = aDir.clone();
            metaFile.append("metadata.json");
            DOMApplicationRegistry._loadJSONAsync(metaFile, function(aMetadata) {
              if (!aMetadata) {
                self._sendError("Error Parsing metadata.json", aId);
                return;
              }

              if (!aMetadata.origin) {
                self._sendError("Missing 'origin' property in metadata.json", aId);
                return;
              }

              let origin = aMetadata.origin;
              let manifestURL = aMetadata.manifestURL ||
                                origin + "/manifest.webapp";
              
              let app = {
                origin: origin,
                installOrigin: aMetadata.installOrigin || origin,
                manifestURL: manifestURL,
                appStatus: appType,
                receipts: aReceipts,
              };

              self._registerApp(app, aId, aDir);
            });
          });
        } catch(e) {
          
          self._sendError(e.toString(), aId);
        }
      }
    }

    Services.tm.currentThread.dispatch(runnable,
                                       Ci.nsIThread.DISPATCH_NORMAL);
  },

  installPackagedApp: function wa_actorInstallPackaged(aDir, aId, aReceipts) {
    debug("installPackagedApp");
    let self = this;

    let runnable = {
      run: function run() {
        try {
          
          let installDir = DOMApplicationRegistry._getAppDir(aId);

          
          
          let zipFile = aDir.clone();
          zipFile.append("application.zip");
          let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"]
                            .createInstance(Ci.nsIZipReader);
          zipReader.open(zipFile);
          let manFile = installDir.clone();
          manFile.append("manifest.webapp");
          zipReader.extract("manifest.webapp", manFile);
          zipReader.close();
          zipFile.moveTo(installDir, "application.zip");

          DOMApplicationRegistry._loadJSONAsync(manFile, function(aManifest) {
            if (!aManifest) {
              self._sendError("Error Parsing manifest.webapp", aId);
            }

            let appType = self._getAppType(aManifest.type);

            
            if (!DOMApplicationRegistry.allowSideloadingCertified &&
                appType == Ci.nsIPrincipal.APP_STATUS_CERTIFIED) {
              self._sendError("Installing certified apps is not allowed.", aId);
              return;
            }

            let origin = "app://" + aId;

            
            let app = {
              origin: origin,
              installOrigin: origin,
              manifestURL: origin + "/manifest.webapp",
              appStatus: appType,
              receipts: aReceipts,
            }

            self._registerApp(app, aId, aDir);
          });
        } catch(e) {
          
          self._sendError(e.toString(), aId);
        }
      }
    }

    Services.tm.currentThread.dispatch(runnable,
                                       Ci.nsIThread.DISPATCH_NORMAL);
  },

  





  install: function wa_actorInstall(aRequest) {
    debug("install");

    let appId = aRequest.appId;
    if (!appId) {
      return { error: "missingParameter",
               message: "missing parameter appId" }
    }

    
    let reg = DOMApplicationRegistry;
    if (appId in reg.webapps && reg.webapps[appId].removable === false) {
      return { error: "badParameterType",
               message: "The application " + appId + " can't be overriden."
             }
    }

    let appDir = FileUtils.getDir("TmpD", ["b2g", appId], false, false);

    if (!appDir || !appDir.exists()) {
      return { error: "badParameterType",
               message: "missing directory " + appDir.path
             }
    }

    let testFile = appDir.clone();
    testFile.append("application.zip");

    let receipts = (aRequest.receipts && Array.isArray(aRequest.receipts))
                    ? aRequest.receipts
                    : [];

    if (testFile.exists()) {
      this.installPackagedApp(appDir, appId, receipts);
    } else {
      let missing =
        ["manifest.webapp", "metadata.json"]
        .some(function(aName) {
          testFile = appDir.clone();
          testFile.append(aName);
          return !testFile.exists();
        });

      if (missing) {
        try {
          appDir.remove(true);
        } catch(e) {}
        return { error: "badParameterType",
                 message: "hosted app file is missing" }
      }

      this.installHostedApp(appDir, appId, receipts);
    }

    return { appId: appId, path: appDir.path }
  },

  getAll: function wa_actorGetAll(aRequest) {
    debug("getAll");

    let defer = Promise.defer();
    let reg = DOMApplicationRegistry;
    reg.getAll(function onsuccess(apps) {
      defer.resolve({ apps: apps });
    });

    return defer.promise;
  },

  uninstall: function wa_actorUninstall(aRequest) {
    debug("uninstall");

    let manifestURL = aRequest.manifestURL;
    if (!manifestURL) {
      return { error: "missingParameter",
               message: "missing parameter manifestURL" };
    }

    let defer = Promise.defer();
    let reg = DOMApplicationRegistry;
    reg.uninstall(
      manifestURL,
      function onsuccess() {
        defer.resolve({});
      },
      function onfailure(reason) {
        defer.resolve({ error: reason });
      }
    );

    return defer.promise;
  },

  launch: function wa_actorLaunch(aRequest) {
    debug("launch");

    let manifestURL = aRequest.manifestURL;
    if (!manifestURL) {
      return { error: "missingParameter",
               message: "missing parameter manifestURL" };
    }

    let defer = Promise.defer();
    let reg = DOMApplicationRegistry;
    reg.launch(
      aRequest.manifestURL,
      aRequest.startPoint || "",
      function onsuccess() {
        defer.resolve({});
      },
      function onfailure(reason) {
        defer.resolve({ error: reason });
      });

    return defer.promise;
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
  }
};




WebappsActor.prototype.requestTypes = {
  "install": WebappsActor.prototype.install,
  "getAll": WebappsActor.prototype.getAll,
  "launch": WebappsActor.prototype.launch,
  "close": WebappsActor.prototype.close,
  "uninstall": WebappsActor.prototype.uninstall
};

DebuggerServer.addGlobalActor(WebappsActor, "webappsActor");

