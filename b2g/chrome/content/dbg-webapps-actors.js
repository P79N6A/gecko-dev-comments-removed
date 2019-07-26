



"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

function debug(aMsg) {
  




}

#ifdef MOZ_WIDGET_GONK
  const DIRECTORY_NAME = "webappsDir";
#else
  const DIRECTORY_NAME = "ProfD";
#endif





function WebappsActor(aConnection) { debug("init"); }

WebappsActor.prototype = {
  actorPrefix: "webapps",

  _registerApp: function wa_actorRegisterApp(aApp, aId, aDir) {
    let reg = DOMApplicationRegistry;
    let self = this;

    aApp.installTime = Date.now();
    aApp.installState = "installed";
    aApp.removable = true;
    aApp.id = aId;
    aApp.basePath = FileUtils.getDir(DIRECTORY_NAME, ["webapps"], true).path;
    aApp.localId = (aId in reg.webapps) ? reg.webapps[aId].localId
                                        : reg._nextLocalId();

    reg.webapps[aId] = aApp;
    reg.updatePermissionsForApp(aId);

    reg._readManifests([{ id: aId }], function(aResult) {
      let manifest = aResult[0].manifest;
      aApp.name = manifest.name;
      reg._registerSystemMessages(manifest, aApp);
      reg._registerActivities(manifest, aApp, true);
      reg._saveApps(function() {
        aApp.manifest = manifest;
        reg.broadcastMessage("Webapps:Install:Return:OK",
                             { app: aApp,
                               oid: "foo",
                               requestID: "bar"
                             });
        delete aApp.manifest;
        reg.broadcastMessage("Webapps:AddApp", { id: aId, app: aApp });
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

  installHostedApp: function wa_actorInstallHosted(aDir, aId, aType) {
    debug("installHostedApp");
    let self = this;

    let runnable = {
      run: function run() {
        try {
          
          let installDir = FileUtils.getDir(DIRECTORY_NAME,
                                            ["webapps", aId], true);

          
          let manFile = aDir.clone();
          manFile.append("manifest.webapp");
          manFile.moveTo(installDir, "manifest.webapp");

          
          let metaFile = aDir.clone();
          metaFile.append("metadata.json");
          DOMApplicationRegistry._loadJSONAsync(metaFile, function(aMetadata) {
            if (!aMetadata) {
              self._sendError("Error Parsing metadata.json", aId);
              return;
            }

            if (!aMetadata.origin) {
              self._sendError("Missing 'origin' propery in metadata.json", aId);
              return;
            }

            let origin = aMetadata.origin;
            let manifestURL = aMetadata.manifestURL ||
                              origin + "/manifest.webapp";
            
            let app = {
              origin: origin,
              installOrigin: aMetadata.installOrigin || origin,
              manifestURL: manifestURL,
              appStatus: aType
            }

            self._registerApp(app, aId, aDir);
          });
        } catch(e) {
          
          self.sendError(e.toString());
        }
      }
    }

    Services.tm.currentThread.dispatch(runnable,
                                       Ci.nsIThread.DISPATCH_NORMAL);
  },

  installPackagedApp: function wa_actorInstallPackaged(aDir, aId, aType) {
    debug("installPackagedApp");
    let self = this;

    let runnable = {
      run: function run() {
        try {
          
          let installDir = FileUtils.getDir(DIRECTORY_NAME,
                                            ["webapps", aId], true);

          
          let zipFile = aDir.clone();
          zipFile.append("application.zip");
          zipFile.moveTo(installDir, "application.zip");

          
          zipFile = installDir.clone();
          zipFile.append("application.zip");
          let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"]
                            .createInstance(Ci.nsIZipReader);
          zipReader.open(zipFile);

          let manFile = installDir.clone();
          manFile.append("manifest.webapp");
          zipReader.extract("manifest.webapp", manFile);
          zipReader.close();

          let origin = "app://" + aId;

          
          let app = {
            origin: origin,
            installOrigin: origin,
            manifestURL: origin + "/manifest.webapp",
            appStatus: aType
          }

          self._registerApp(app, aId, aDir);
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

    Cu.import("resource://gre/modules/Webapps.jsm");
    Cu.import("resource://gre/modules/AppsUtils.jsm");
    Cu.import("resource://gre/modules/FileUtils.jsm");

    let appId = aRequest.appId;
    if (!appId) {
      return { error: "missingParameter",
               message: "missing parameter appId" }
    }

    let appType = aRequest.appType || Ci.nsIPrincipal.APP_STATUS_INSTALLED;
    let appDir = FileUtils.getDir("TmpD", ["b2g", appId], false, false);

    if (!appDir || !appDir.exists()) {
      return { error: "badParameterType",
               message: "missing directory " + appDir.path
             }
    }

    let testFile = appDir.clone();
    testFile.append("application.zip");

    if (testFile.exists()) {
      this.installPackagedApp(appDir, appId, appType);
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
          aDir.remove(true);
        } catch(e) {}
        return { error: "badParameterType",
                 message: "hosted app file is missing" }
      }

      this.installHostedApp(appDir, appId, appType);
    }

    return { appId: appId, path: appDir.path }
  }
};




WebappsActor.prototype.requestTypes = {
  "install": WebappsActor.prototype.install
};

DebuggerServer.addGlobalActor(WebappsActor, "webappsActor");
