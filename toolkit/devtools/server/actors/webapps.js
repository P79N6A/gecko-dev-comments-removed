



"use strict";

let Cu = Components.utils;
let Cc = Components.classes;
let Ci = Components.interfaces;

let promise;

function debug(aMsg) {
  




}





function WebappsActor(aConnection) {
  debug("init");
  
  

  Cu.import("resource://gre/modules/Webapps.jsm");
  Cu.import("resource://gre/modules/AppsUtils.jsm");
  Cu.import("resource://gre/modules/FileUtils.jsm");
  Cu.import('resource://gre/modules/Services.jsm');
  promise = Cu.import("resource://gre/modules/commonjs/sdk/core/promise.js").Promise;

  
  
  this._appActorsMap = new Map();
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
      reg.updateAppHandlers(null, manifest, aApp);

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

    let defer = promise.defer();
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

    let defer = promise.defer();
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

    let defer = promise.defer();
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
  },

  _appFrames: function () {
    
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

    let apps = [];

    for each (let frame in this._appFrames()) {
      let manifestURL = frame.getAttribute("mozapp");
      apps.push(manifestURL);
    }

    return { apps: apps };
  },

  _connectToApp: function (aFrame) {
    let defer = Promise.defer();

    let mm = aFrame.QueryInterface(Ci.nsIFrameLoaderOwner).frameLoader.messageManager;
    mm.loadFrameScript("resource://gre/modules/devtools/server/child.js", false);

    let childTransport, prefix;

    let onActorCreated = makeInfallible(function (msg) {
      mm.removeMessageListener("debug:actor", onActorCreated);

      dump("***** Got debug:actor\n");
      let { actor, appId } = msg.json;
      prefix = msg.json.prefix;

      
      childTransport = new ChildDebuggerTransport(mm, prefix);
      childTransport.hooks = {
        onPacket: this.conn.send.bind(this.conn),
        onClosed: function () {}
      };
      childTransport.ready();

      this.conn.setForwarding(prefix, childTransport);

      debug("establishing forwarding for app with prefix " + prefix);

      this._appActorsMap.set(mm, actor);

      defer.resolve(actor);
    }).bind(this);
    mm.addMessageListener("debug:actor", onActorCreated);

    let onMessageManagerDisconnect = makeInfallible(function (subject, topic, data) {
      if (subject == mm) {
        Services.obs.removeObserver(onMessageManagerDisconnect, topic);
        if (childTransport) {
          
          
          childTransport.close();
          this.conn.cancelForwarding(prefix);
        } else {
          
          
          
          defer.resolve(null);
        }
        this._appActorsMap.delete(mm);
      }
    }).bind(this);
    Services.obs.addObserver(onMessageManagerDisconnect,
                             "message-manager-disconnect", false);

    let prefixStart = this.conn.prefix + "child";
    mm.sendAsyncMessage("debug:connect", { prefix: prefixStart });

    return defer.promise;
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

    if (!appFrame) {
      return { error: "appNotFound",
               message: "Unable to find any opened app whose manifest " +
                        "is '" + manifestURL + "'" };
    }

    
    
    let mm = appFrame.QueryInterface(Ci.nsIFrameLoaderOwner)
                     .frameLoader
                     .messageManager;
    let actor = this._appActorsMap.get(mm);
    if (!actor) {
      return this._connectToApp(appFrame)
                 .then(function (actor) ({ actor: actor }));
    }

    return { actor: actor };
  },

  watchApps: function () {
    this._framesByOrigin = {};
    let chromeWindow = Services.wm.getMostRecentWindow('navigator:browser');
    let systemAppFrame = chromeWindow.getContentWindow();
    systemAppFrame.addEventListener("appwillopen", this);
    systemAppFrame.addEventListener("appterminated", this);

    return {};
  },

  unwatchApps: function () {
    this._framesByOrigin = null;
    let chromeWindow = Services.wm.getMostRecentWindow('navigator:browser');
    let systemAppFrame = chromeWindow.getContentWindow();
    systemAppFrame.removeEventListener("appwillopen", this);
    systemAppFrame.removeEventListener("appterminated", this);

    return {};
  },

  handleEvent: function (event) {
    let frame;
    let origin = event.detail.origin;
    switch(event.type) {
      case "appwillopen":
        frame = event.target;
        
        
        
        let mm = frame.QueryInterface(Ci.nsIFrameLoaderOwner)
                         .frameLoader
                         .messageManager;
        if (this._appActorsMap.has(mm)) {
          return;
        }

        
        
        
        
        this._framesByOrigin[origin] = frame;

        this.conn.send({ from: this.actorID,
                         type: "appOpen",
                         manifestURL: frame.getAttribute("mozapp")
                       });
        break;

      case "appterminated":
        
        
        
        frame = this._framesByOrigin[origin];
        delete this._framesByOrigin[origin];
        if (frame) {
          let manifestURL = frame.getAttribute("mozapp");
          this.conn.send({ from: this.actorID,
                           type: "appClose",
                           manifestURL: manifestURL
                         });
        }
        break;
    }
  }
};




WebappsActor.prototype.requestTypes = {
  "install": WebappsActor.prototype.install
};



if (Services.prefs.getBoolPref("devtools.debugger.enable-content-actors")) {
  let requestTypes = WebappsActor.prototype.requestTypes;
  requestTypes.getAll = WebappsActor.prototype.getAll;
  requestTypes.launch = WebappsActor.prototype.launch;
  requestTypes.close  = WebappsActor.prototype.close;
  requestTypes.uninstall = WebappsActor.prototype.uninstall;
  requestTypes.listRunningApps = WebappsActor.prototype.listRunningApps;
  requestTypes.getAppActor = WebappsActor.prototype.getAppActor;
  requestTypes.watchApps = WebappsActor.prototype.watchApps;
  requestTypes.unwatchApps = WebappsActor.prototype.unwatchApps;
}

DebuggerServer.addGlobalActor(WebappsActor, "webappsActor");
