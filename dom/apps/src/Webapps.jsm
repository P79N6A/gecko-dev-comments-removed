



"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

this.EXPORTED_SYMBOLS = ["DOMApplicationRegistry"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import('resource://gre/modules/ActivitiesService.jsm');
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/PermissionsInstaller.jsm");
Cu.import("resource://gre/modules/OfflineCacheInstaller.jsm");
Cu.import("resource://gre/modules/SystemMessagePermissionsChecker.jsm");
Cu.import("resource://gre/modules/AppDownloadManager.jsm");

function debug(aMsg) {
  
}


const MIN_PROGRESS_EVENT_DELAY = 1000;

const WEBAPP_RUNTIME = Services.appinfo.ID == "webapprt@mozilla.org";

XPCOMUtils.defineLazyGetter(this, "NetUtil", function() {
  Cu.import("resource://gre/modules/NetUtil.jsm");
  return NetUtil;
});

XPCOMUtils.defineLazyServiceGetter(this, "ppmm",
                                   "@mozilla.org/parentprocessmessagemanager;1",
                                   "nsIMessageBroadcaster");

XPCOMUtils.defineLazyServiceGetter(this, "cpmm",
                                   "@mozilla.org/childprocessmessagemanager;1",
                                   "nsIMessageSender");

XPCOMUtils.defineLazyGetter(this, "msgmgr", function() {
  return Cc["@mozilla.org/system-message-internal;1"]
         .getService(Ci.nsISystemMessagesInternal);
});

XPCOMUtils.defineLazyGetter(this, "updateSvc", function() {
  return Cc["@mozilla.org/offlinecacheupdate-service;1"]
           .getService(Ci.nsIOfflineCacheUpdateService);
});

#ifdef MOZ_WIDGET_GONK
  const DIRECTORY_NAME = "webappsDir";
#elifdef ANDROID
  const DIRECTORY_NAME = "webappsDir";
#else
  
  
  
  const DIRECTORY_NAME = WEBAPP_RUNTIME ? "WebappRegD" : "ProfD";
#endif

this.DOMApplicationRegistry = {
  appsFile: null,
  webapps: { },
  children: [ ],
  allAppsLaunchable: false,

  init: function() {
    this.messages = ["Webapps:Install", "Webapps:Uninstall",
                     "Webapps:GetSelf", "Webapps:CheckInstalled",
                     "Webapps:GetInstalled", "Webapps:GetNotInstalled",
                     "Webapps:Launch", "Webapps:GetAll",
                     "Webapps:InstallPackage",
                     "Webapps:GetList", "Webapps:RegisterForMessages",
                     "Webapps:UnregisterForMessages",
                     "Webapps:CancelDownload", "Webapps:CheckForUpdate",
                     "Webapps:Download", "Webapps:ApplyDownload",
                     "Webapps:Install:Return:Ack",
                     "child-process-shutdown"];

    this.frameMessages = ["Webapps:ClearBrowserData"];

    this.messages.forEach((function(msgName) {
      ppmm.addMessageListener(msgName, this);
    }).bind(this));

    cpmm.addMessageListener("Activities:Register:OK", this);

    Services.obs.addObserver(this, "xpcom-shutdown", false);

    AppDownloadManager.registerCancelFunction(this.cancelDownload.bind(this));

    this.appsFile = FileUtils.getFile(DIRECTORY_NAME,
                                      ["webapps", "webapps.json"], true);

    this.loadAndUpdateApps();
  },

  
  
  loadCurrentRegistry: function loadCurrentRegistry(aNext) {
    let file = FileUtils.getFile(DIRECTORY_NAME, ["webapps", "webapps.json"], false);
    if (file && file.exists()) {
      this._loadJSONAsync(file, (function loadRegistry(aData) {
        if (aData) {
          this.webapps = aData;
          let appDir = FileUtils.getDir(DIRECTORY_NAME, ["webapps"], false);
          for (let id in this.webapps) {
            let app = this.webapps[id];

            app.id = id;

            
            if (app.localId === undefined) {
              app.localId = this._nextLocalId();
            }

            if (app.basePath === undefined) {
              app.basePath = appDir.path;
            }

            
            if (app.removable === undefined) {
              app.removable = true;
            }

            
            if (app.appStatus === undefined) {
              app.appStatus = Ci.nsIPrincipal.APP_STATUS_INSTALLED;
            }

            
            if (app.installerAppId === undefined) {
              app.installerAppId = Ci.nsIScriptSecurityManager.NO_APP_ID;
            }
            if (app.installerIsBrowser === undefined) {
              app.installerIsBrowser = false;
            }

            
            
            if (app.installState === undefined ||
                app.installState === "updating") {
              app.installState = "installed";
            }

            
            
            app.downloading = false;
            app.readyToApplyDownload = false;
          };
        }
        aNext();
      }).bind(this));
    } else {
      aNext();
    }
  },

  
  notifyAppsRegistryStart: function notifyAppsRegistryStart() {
    Services.obs.notifyObservers(this, "webapps-registry-start", null);
  },

  
  notifyAppsRegistryReady: function notifyAppsRegistryReady() {
    Services.obs.notifyObservers(this, "webapps-registry-ready", null);
    this._saveApps();
  },

  
  registerAppsHandlers: function registerAppsHandlers(aRunUpdate) {
    this.notifyAppsRegistryStart();
    let ids = [];
    for (let id in this.webapps) {
      ids.push({ id: id });
    }
#ifdef MOZ_SYS_MSG
    this._processManifestForIds(ids, aRunUpdate);
#else
    
    
    
    this._readManifests(ids, (function readCSPs(aResults) {
      aResults.forEach(function registerManifest(aResult) {
        this.webapps[aResult.id].csp = aResult.manifest.csp || "";
      }, this);
    }).bind(this));

    
    this.notifyAppsRegistryReady();
#endif
  },

  updatePermissionsForApp: function updatePermissionsForApp(aId) {
    if (!this.webapps[aId]) {
      return;
    }

    
    
    this._readManifests([{ id: aId }], (function(aResult) {
      let data = aResult[0];
      PermissionsInstaller.installPermissions({
        manifest: data.manifest,
        manifestURL: this.webapps[aId].manifestURL,
        origin: this.webapps[aId].origin
      }, true, function() {
        debug("Error installing permissions for " + aId);
      });
    }).bind(this));
  },

  updateOfflineCacheForApp: function updateOfflineCacheForApp(aId) {
    let app = this.webapps[aId];
    OfflineCacheInstaller.installCache({
      basePath: app.basePath,
      appId: aId,
      origin: app.origin,
      localId: app.localId
    });
  },

  
  installPreinstalledApp: function installPreinstalledApp(aId) {
#ifdef MOZ_WIDGET_GONK
    let app = this.webapps[aId];
    let baseDir;
    try {
      baseDir = FileUtils.getDir("coreAppsDir", ["webapps", aId], false);
      if (!baseDir.exists()) {
        return;
      }
    } catch(e) {
      
      return;
    }

    let filesToMove;
    let isPackage;

    let updateFile = baseDir.clone();
    updateFile.append("update.webapp");
    if (!updateFile.exists()) {
      
      
      let appFile = baseDir.clone();
      appFile.append("application.zip");
      if (appFile.exists()) {
        return;
      }

      isPackage = false;
      filesToMove = ["manifest.webapp"];
    } else {
      isPackage = true;
      filesToMove = ["application.zip", "update.webapp"];
    }

    debug("Installing 3rd party app : " + aId +
          " from " + baseDir.path);

    
    let destDir = FileUtils.getDir(DIRECTORY_NAME, ["webapps", aId], true, true);

    filesToMove.forEach(function(aFile) {
        let file = baseDir.clone();
        file.append(aFile);
        file.copyTo(destDir, aFile);
      });

    app.installState = "installed";
    app.basePath = FileUtils.getDir(DIRECTORY_NAME, ["webapps"], true, true)
                            .path;

    if (!isPackage) {
      return;
    }

    app.origin = "app://" + aId;
    app.removable = true;

    
    let zipFile = baseDir.clone();
    zipFile.append("application.zip");
    let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"]
                      .createInstance(Ci.nsIZipReader);
    try {
      debug("Opening " + zipFile.path);
      zipReader.open(zipFile);
      if (!zipReader.hasEntry("manifest.webapp")) {
        throw "MISSING_MANIFEST";
      }
      let manifestFile = destDir.clone();
      manifestFile.append("manifest.webapp");
      zipReader.extract("manifest.webapp", manifestFile);
    } catch(e) {
      
      debug("Cleaning up: " + e);
      destDir.remove(true);
      delete this.webapps[aId];
    } finally {
      zipReader.close();
    }
#endif
  },

  
  
  
  
  
  
  
  installSystemApps: function installSystemApps(aNext) {
    let file;
    try {
      file = FileUtils.getFile("coreAppsDir", ["webapps", "webapps.json"], false);
    } catch(e) { }

    if (file && file.exists()) {
      
      this._loadJSONAsync(file, (function loadCoreRegistry(aData) {
        if (!aData) {
          aNext();
          return;
        }

        
        for (let id in this.webapps) {
          if (id in aData || this.webapps[id].removable)
            continue;
          delete this.webapps[id];
          
          let localId = this.webapps[id].localId;
          let permMgr = Cc["@mozilla.org/permissionmanager;1"]
                          .getService(Ci.nsIPermissionManager);
          permMgr.RemovePermissionsForApp(localId, false);
          Services.cookies.removeCookiesForApp(localId, false);
          this._clearPrivateData(localId, false);
        }

        let appDir = FileUtils.getDir("coreAppsDir", ["webapps"], false);
        
        for (let id in aData) {
          
          
          if (!(id in this.webapps)) {
            this.webapps[id] = aData[id];
            this.webapps[id].basePath = appDir.path;

            this.webapps[id].id = id;

            
            this.webapps[id].localId = this._nextLocalId();

            
            if (this.webapps[id].removable === undefined) {
              this.webapps[id].removable = false;
            }
          }
        }
        aNext();
      }).bind(this));
    } else {
      aNext();
    }
  },

#ifdef MOZ_WIDGET_GONK
  fixIndexedDb: function fixIndexedDb() {
    debug("Fixing indexedDb folder names");
    let idbDir = FileUtils.getDir("indexedDBPDir", ["indexedDB"]);

    if (!idbDir.isDirectory()) {
      return;
    }

    let re = /^(\d+)\+(.*)\+(f|t)$/;

    let entries = idbDir.directoryEntries;
    while (entries.hasMoreElements()) {
      let entry = entries.getNext().QueryInterface(Ci.nsIFile);
      if (!entry.isDirectory()) {
        continue;
      }

      let newName = entry.leafName.replace(re, "$1+$3+$2");
      if (newName != entry.leafName) {
        try {
          entry.moveTo(idbDir, newName);
        } catch(e) { }
      }
    }
  },
#endif

  loadAndUpdateApps: function loadAndUpdateApps() {
    let runUpdate = AppsUtils.isFirstRun(Services.prefs);

#ifdef MOZ_WIDGET_GONK
    if (runUpdate) {
      this.fixIndexedDb();
    }
#endif

    let onAppsLoaded = (function onAppsLoaded() {
      if (runUpdate) {
        
        for (let id in this.webapps) {
          this.installPreinstalledApp(id);
          if (!this.webapps[id]) {
            continue;
          }
          this.updateOfflineCacheForApp(id);
          this.updatePermissionsForApp(id);
        }
        
        
        this._saveApps();
      }
      this.registerAppsHandlers(runUpdate);
    }).bind(this);

    this.loadCurrentRegistry((function() {
#ifdef MOZ_WIDGET_GONK
      
      if (runUpdate)
        this.installSystemApps(onAppsLoaded);
      else
        onAppsLoaded();
#else
      onAppsLoaded();
#endif
    }).bind(this));
  },

#ifdef MOZ_SYS_MSG
  
  
  _registerSystemMessagesForEntryPoint: function(aManifest, aApp, aEntryPoint) {
    let root = aManifest;
    if (aEntryPoint && aManifest.entry_points[aEntryPoint]) {
      root = aManifest.entry_points[aEntryPoint];
    }

    if (!root.messages || !Array.isArray(root.messages) ||
        root.messages.length == 0) {
      return;
    }

    let manifest = new ManifestHelper(aManifest, aApp.origin);
    let launchPath = Services.io.newURI(manifest.fullLaunchPath(aEntryPoint), null, null);
    let manifestURL = Services.io.newURI(aApp.manifestURL, null, null);
    root.messages.forEach(function registerPages(aMessage) {
      let href = launchPath;
      let messageName;
      if (typeof(aMessage) === "object" && Object.keys(aMessage).length === 1) {
        messageName = Object.keys(aMessage)[0];
        let uri;
        try {
          uri = manifest.resolveFromOrigin(aMessage[messageName]);
        } catch(e) {
          debug("system message url (" + aMessage[messageName] + ") is invalid, skipping. " +
                "Error is: " + e);
          return;
        }
        href = Services.io.newURI(uri, null, null);
      } else {
        messageName = aMessage;
      }

      if (SystemMessagePermissionsChecker
            .isSystemMessagePermittedToRegister(messageName,
                                                aApp.origin,
                                                aManifest)) {
        msgmgr.registerPage(messageName, href, manifestURL);
      }
    });
  },

  _registerSystemMessages: function(aManifest, aApp) {
    this._registerSystemMessagesForEntryPoint(aManifest, aApp, null);

    if (!aManifest.entry_points) {
      return;
    }

    for (let entryPoint in aManifest.entry_points) {
      this._registerSystemMessagesForEntryPoint(aManifest, aApp, entryPoint);
    }
  },

  
  
  _createActivitiesToRegister: function(aManifest, aApp, aEntryPoint, aRunUpdate) {
    let activitiesToRegister = [];
    let root = aManifest;
    if (aEntryPoint && aManifest.entry_points[aEntryPoint]) {
      root = aManifest.entry_points[aEntryPoint];
    }

    if (!root.activities) {
      return activitiesToRegister;
    }

    let manifest = new ManifestHelper(aManifest, aApp.origin);
    for (let activity in root.activities) {
      let description = root.activities[activity];
      let href = description.href;
      if (!href) {
        href = manifest.launch_path;
      }

      try {
        href = manifest.resolveFromOrigin(href);
      } catch (e) {
        debug("Activity href (" + href + ") is invalid, skipping. " +
              "Error is: " + e);
        continue;
      }

      
      
      let newDesc = {};
      for (let prop in description) {
        newDesc[prop] = description[prop];
      }
      newDesc.href = href;

      debug('_createActivitiesToRegister: ' + aApp.manifestURL + ', activity ' +
          activity + ', description.href is ' + newDesc.href);

      if (aRunUpdate) {
        activitiesToRegister.push({ "manifest": aApp.manifestURL,
                                    "name": activity,
                                    "icon": manifest.iconURLForSize(128),
                                    "description": newDesc });
      }

      let launchPath = Services.io.newURI(href, null, null);
      let manifestURL = Services.io.newURI(aApp.manifestURL, null, null);

      if (SystemMessagePermissionsChecker
            .isSystemMessagePermittedToRegister("activity",
                                                aApp.origin,
                                                aManifest)) {
        msgmgr.registerPage("activity", launchPath, manifestURL);
      }
    }
    return activitiesToRegister;
  },

  
  
  _registerActivitiesForApps: function(aAppsToRegister, aRunUpdate) {
    
    let activitiesToRegister = [];
    aAppsToRegister.forEach(function (aApp) {
      let manifest = aApp.manifest;
      let app = aApp.app;
      activitiesToRegister.push.apply(activitiesToRegister,
        this._createActivitiesToRegister(manifest, app, null, aRunUpdate));

      if (!manifest.entry_points) {
        return;
      }

      for (let entryPoint in manifest.entry_points) {
        activitiesToRegister.push.apply(activitiesToRegister,
          this._createActivitiesToRegister(manifest, app, entryPoint, aRunUpdate));
      }
    }, this);

    if (!aRunUpdate || activitiesToRegister.length == 0) {
      this.notifyAppsRegistryReady();
      return;
    }

    
    cpmm.sendAsyncMessage("Activities:Register", activitiesToRegister);
  },

  
  
  _registerActivities: function(aManifest, aApp, aRunUpdate) {
    this._registerActivitiesForApps([{ manifest: aManifest, app: aApp }], aRunUpdate);
  },

  
  
  _createActivitiesToUnregister: function(aManifest, aApp, aEntryPoint) {
    let activitiesToUnregister = [];
    let root = aManifest;
    if (aEntryPoint && aManifest.entry_points[aEntryPoint]) {
      root = aManifest.entry_points[aEntryPoint];
    }

    if (!root.activities) {
      return activitiesToUnregister;
    }

    for (let activity in root.activities) {
      let description = root.activities[activity];
      activitiesToUnregister.push({ "manifest": aApp.manifestURL,
                                    "name": activity });
    }
    return activitiesToUnregister;
  },

  
  
  _unregisterActivitiesForApps: function(aAppsToUnregister) {
    
    let activitiesToUnregister = [];
    aAppsToUnregister.forEach(function (aApp) {
      let manifest = aApp.manifest;
      let app = aApp.app;
      activitiesToUnregister.push.apply(activitiesToUnregister,
        this._createActivitiesToUnregister(manifest, app, null));

      if (!manifest.entry_points) {
        return;
      }

      for (let entryPoint in manifest.entry_points) {
        activitiesToUnregister.push.apply(activitiesToUnregister,
          this._createActivitiesToUnregister(manifest, app, entryPoint));
      }
    }, this);

    
    cpmm.sendAsyncMessage("Activities:Unregister", activitiesToUnregister);
  },

  
  
  _unregisterActivities: function(aManifest, aApp) {
    this._unregisterActivitiesForApps([{ manifest: aManifest, app: aApp }]);
  },

  _processManifestForIds: function(aIds, aRunUpdate) {
    this._readManifests(aIds, (function registerManifests(aResults) {
      let appsToRegister = [];
      aResults.forEach(function registerManifest(aResult) {
        let app = this.webapps[aResult.id];
        let manifest = aResult.manifest;
        app.name = manifest.name;
        app.csp = manifest.csp || "";
        this._registerSystemMessages(manifest, app);
        appsToRegister.push({ manifest: manifest, app: app });
      }, this);
      this._registerActivitiesForApps(appsToRegister, aRunUpdate);
    }).bind(this));
  },
#endif

  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "xpcom-shutdown") {
      this.messages.forEach((function(msgName) {
        ppmm.removeMessageListener(msgName, this);
      }).bind(this));
      Services.obs.removeObserver(this, "xpcom-shutdown");
      ppmm = null;
    }
  },

  _loadJSONAsync: function(aFile, aCallback) {
    try {
      let channel = NetUtil.newChannel(aFile);
      channel.contentType = "application/json";
      NetUtil.asyncFetch(channel, function(aStream, aResult) {
        if (!Components.isSuccessCode(aResult)) {
          Cu.reportError("DOMApplicationRegistry: Could not read from json file "
                         + aFile.path);
          if (aCallback)
            aCallback(null);
          return;
        }

        
        let data = null;
        try {
          
          let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                          .createInstance(Ci.nsIScriptableUnicodeConverter);
          converter.charset = "UTF-8";

          data = JSON.parse(converter.ConvertToUnicode(NetUtil.readInputStreamToString(aStream,
                                                            aStream.available()) || ""));
          aStream.close();
          if (aCallback)
            aCallback(data);
        } catch (ex) {
          Cu.reportError("DOMApplicationRegistry: Could not parse JSON: " +
                         aFile.path + " " + ex + "\n" + ex.stack);
          if (aCallback)
            aCallback(null);
        }
      });
    } catch (ex) {
      Cu.reportError("DOMApplicationRegistry: Could not read from " +
                     aFile.path + " : " + ex + "\n" + ex.stack);
      if (aCallback)
        aCallback(null);
    }
  },

  addMessageListener: function(aMsgNames, aMm) {
    aMsgNames.forEach(function (aMsgName) {
      if (!(aMsgName in this.children)) {
        this.children[aMsgName] = [];
      }

      let mmFound = this.children[aMsgName].some(function(mmRef) {
        if (mmRef.mm === aMm) {
          mmRef.refCount++;
          return true;
        }
        return false;
      });

      if (!mmFound) {
        this.children[aMsgName].push({
          mm: aMm,
          refCount: 1
        });
      }
    }, this);
  },

  removeMessageListener: function(aMsgNames, aMm) {
    if (aMsgNames.length === 1 &&
        aMsgNames[0] === "Webapps:Internal:AllMessages") {
      for (let msgName in this.children) {
        let msg = this.children[msgName];

        for (let mmI = msg.length - 1; mmI >= 0; mmI -= 1) {
          let mmRef = msg[mmI];
          if (mmRef.mm === aMm) {
            msg.splice(mmI, 1);
          }
        }

        if (msg.length === 0) {
          delete this.children[msgName];
        }
      }
      return;
    }

    aMsgNames.forEach(function(aMsgName) {
      if (!(aMsgName in this.children)) {
        return;
      }

      let removeIndex;
      this.children[aMsgName].some(function(mmRef, index) {
        if (mmRef.mm === aMm) {
          mmRef.refCount--;
          if (mmRef.refCount === 0) {
            removeIndex = index;
          }
          return true;
        }
        return false;
      });

      if (removeIndex) {
        this.children[aMsgName].splice(removeIndex, 1);
      }
    }, this);
  },

  receiveMessage: function(aMessage) {
    
    
    Services.prefs.setBoolPref("dom.mozApps.used", true);

    
    
    if (["Webapps:GetAll",
         "Webapps:GetNotInstalled",
         "Webapps:ApplyDownload",
         "Webapps:Uninstall"].indexOf(aMessage.name) != -1) {
      if (!aMessage.target.assertPermission("webapps-manage")) {
        debug("mozApps message " + aMessage.name +
        " from a content process with no 'webapps-manage' privileges.");
        return null;
      }
    }

    let msg = aMessage.data || {};
    let mm = aMessage.target;
    msg.mm = mm;

    switch (aMessage.name) {
      case "Webapps:Install":
        this.doInstall(msg, mm);
        break;
      case "Webapps:GetSelf":
        this.getSelf(msg, mm);
        break;
      case "Webapps:Uninstall":
        this.uninstall(msg, mm);
        break;
      case "Webapps:Launch":
        this.launchApp(msg, mm);
        break;
      case "Webapps:CheckInstalled":
        this.checkInstalled(msg, mm);
        break;
      case "Webapps:GetInstalled":
        this.getInstalled(msg, mm);
        break;
      case "Webapps:GetNotInstalled":
        this.getNotInstalled(msg, mm);
        break;
      case "Webapps:GetAll":
        this.getAll(msg, mm);
        break;
      case "Webapps:InstallPackage":
        this.doInstallPackage(msg, mm);
        break;
      case "Webapps:RegisterForMessages":
        this.addMessageListener(msg, mm);
        break;
      case "Webapps:UnregisterForMessages":
        this.removeMessageListener(msg, mm);
        break;
      case "child-process-shutdown":
        this.removeMessageListener(["Webapps:Internal:AllMessages"], mm);
        break;
      case "Webapps:GetList":
        this.addMessageListener(["Webapps:AddApp", "Webapps:RemoveApp"], mm);
        return this.webapps;
      case "Webapps:Download":
        this.startDownload(msg.manifestURL);
        break;
      case "Webapps:CancelDownload":
        this.cancelDownload(msg.manifestURL);
        break;
      case "Webapps:CheckForUpdate":
        this.checkForUpdate(msg, mm);
        break;
      case "Webapps:ApplyDownload":
        this.applyDownload(msg.manifestURL);
        break;
      case "Activities:Register:OK":
        this.notifyAppsRegistryReady();
        break;
      case "Webapps:Install:Return:Ack":
        this.onInstallSuccessAck(msg.manifestURL);
        break;
    }
  },

  getAppInfo: function getAppInfo(aAppId) {
    if (!this.webapps[aAppId]) {
      debug("No webapp for " + aAppId);
      return null;
    }
    return { "basePath":  this.webapps[aAppId].basePath + "/",
             "isCoreApp": !this.webapps[aAppId].removable };
  },

  
  
  
  
  
  
  
  
  
  broadcastMessage: function broadcastMessage(aMsgName, aContent) {
    if (!(aMsgName in this.children)) {
      return;
    }
    this.children[aMsgName].forEach(function(mmRef) {
      mmRef.mm.sendAsyncMessage(aMsgName, aContent);
    });
  },

  _getAppDir: function(aId) {
    return FileUtils.getDir(DIRECTORY_NAME, ["webapps", aId], true, true);
  },

  _writeFile: function ss_writeFile(aFile, aData, aCallbak) {
    debug("Saving " + aFile.path);
    
    let ostream = FileUtils.openSafeFileOutputStream(aFile);

    
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    
    let istream = converter.convertToInputStream(aData);
    NetUtil.asyncCopy(istream, ostream, function(rc) {
      if (aCallbak)
        aCallbak();
    });
  },

  launchApp: function launchApp(aData, aMm) {
    let app = this.getAppByManifestURL(aData.manifestURL);
    if (!app) {
      aMm.sendAsyncMessage("Webapps:Launch:Return:KO", aData);
      return;
    }

    
    
    if (app.installState == "pending") {
      aMm.sendAsyncMessage("Webapps:Launch:Return:KO", aData);
      return;
    }

    Services.obs.notifyObservers(aMm, "webapps-launch", JSON.stringify(aData));
  },

  cancelDownload: function cancelDownload(aManifestURL, aError) {
    debug("cancelDownload " + aManifestURL);
    let error = aError || "DOWNLOAD_CANCELED";
    let download = AppDownloadManager.get(aManifestURL);
    if (!download) {
      debug("Could not find a download for " + aManifestURL);
      return;
    }

    let app = this.webapps[download.appId];

    if (download.cacheUpdate) {
      
      app.isCanceling = true;
      try {
        download.cacheUpdate.cancel();
      } catch (e) {
        delete app.isCanceling;
        debug (e);
      }
    } else if (download.channel) {
      
      app.isCanceling = true;
      try {
        download.channel.cancel(Cr.NS_BINDING_ABORTED);
      } catch(e) {
        delete app.isCanceling;
      }
    } else {
      return;
    }

    let app = this.webapps[download.appId];
    app.progress = 0;
    app.installState = download.previousState;
    app.downloading = false;
    this._saveApps((function() {
      this.broadcastMessage("Webapps:PackageEvent",
                             { type: "canceled",
                               manifestURL:  app.manifestURL,
                               app: app,
                               error: error });
    }).bind(this));
    AppDownloadManager.remove(aManifestURL);
  },

  startDownload: function startDownload(aManifestURL) {
    debug("startDownload for " + aManifestURL);
    let id = this._appIdForManifestURL(aManifestURL);
    let app = this.webapps[id];
    if (!app) {
      debug("startDownload: No app found for " + aManifestURL);
      return;
    }

    if (app.downloading) {
      debug("app is already downloading. Ignoring.");
      return;
    }

    
    
    if (!app.downloadAvailable) {
      this.broadcastMessage("Webapps:PackageEvent",
                            { type: "canceled",
                              manifestURL: app.manifestURL,
                              app: app,
                              error: "NO_DOWNLOAD_AVAILABLE" });
      return;
    }

    
    
    let isUpdate = (app.installState == "installed");

    
    
    
    app.retryingDownload = !isUpdate;

    
    
    let file = FileUtils.getFile(DIRECTORY_NAME,
                                 ["webapps", id,
                                  isUpdate ? "staged-update.webapp"
                                           : "update.webapp"],
                                 true);

    if (!file.exists()) {
      
      
      this._readManifests([{ id: id }], (function readManifest(aResults) {
        let jsonManifest = aResults[0].manifest;
        let manifest = new ManifestHelper(jsonManifest, app.origin);

        if (manifest.appcache_path) {
          debug("appcache found");
          this.startOfflineCacheDownload(manifest, app, null, null, isUpdate);
        } else {
          
          
          debug("No appcache found, sending 'downloaded' for " + aManifestURL);
          app.downloadAvailable = false;
          DOMApplicationRegistry.broadcastMessage("Webapps:PackageEvent",
                                                  { type: "downloaded",
                                                    manifestURL: aManifestURL,
                                                    app: app,
                                                    manifest: jsonManifest });
          DOMApplicationRegistry._saveApps();
        }
      }).bind(this));

      return;
    }

    this._loadJSONAsync(file, (function(aJSON) {
      if (!aJSON) {
        debug("startDownload: No update manifest found at " + file.path + " " + aManifestURL);
        return;
      }

      let manifest = new ManifestHelper(aJSON, app.installOrigin);
      this.downloadPackage(manifest, {
          manifestURL: aManifestURL,
          origin: app.origin,
          installOrigin: app.installOrigin,
          downloadSize: app.downloadSize
        }, isUpdate, function(aId, aManifest) {
          
          
          let tmpDir = FileUtils.getDir("TmpD", ["webapps", aId], true, true);

          
          let manFile = tmpDir.clone();
          manFile.append("manifest.webapp");
          DOMApplicationRegistry._writeFile(manFile,
                                            JSON.stringify(aManifest),
                                            function() { });
          
          app.downloading = false;
          app.downloadAvailable = false;
          app.readyToApplyDownload = true;
          app.updateTime = Date.now();
          DOMApplicationRegistry._saveApps(function() {
            debug("About to fire Webapps:PackageEvent");
            DOMApplicationRegistry.broadcastMessage("Webapps:PackageEvent",
                                                    { type: "downloaded",
                                                      manifestURL: aManifestURL,
                                                      app: app });
            if (app.installState == "pending") {
              
              DOMApplicationRegistry.applyDownload(aManifestURL);
            }
          });
        });
    }).bind(this));
  },

  applyDownload: function applyDownload(aManifestURL) {
    debug("applyDownload for " + aManifestURL);
    let id = this._appIdForManifestURL(aManifestURL);
    let app = this.webapps[id];
    if (!app || (app && !app.readyToApplyDownload)) {
      return;
    }

    
    if (id in this._manifestCache) {
      delete this._manifestCache[id];
    }

    
    let tmpDir = FileUtils.getDir("TmpD", ["webapps", id], true, true);
    let manFile = tmpDir.clone();
    manFile.append("manifest.webapp");
    let appFile = tmpDir.clone();
    appFile.append("application.zip");

    let dir = FileUtils.getDir(DIRECTORY_NAME, ["webapps", id], true, true);
    appFile.moveTo(dir, "application.zip");
    manFile.moveTo(dir, "manifest.webapp");

    
    let staged = dir.clone();
    staged.append("staged-update.webapp");

    
    
    if (staged.exists()) {
      staged.moveTo(dir, "update.webapp");
    }

    try {
      tmpDir.remove(true);
    } catch(e) { }

    
    
    let zipFile = dir.clone();
    zipFile.append("application.zip");
    Services.obs.notifyObservers(zipFile, "flush-cache-entry", null);

    
    this.getManifestFor(app.origin, (function(aData) {
      app.downloading = false;
      app.downloadAvailable = false;
      app.downloadSize = 0;
      app.installState = "installed";
      app.readyToApplyDownload = false;

      
      if (app.staged) {
        for (let prop in app.staged) {
          app[prop] = app.staged[prop];
        }
        delete app.staged;
      }

      delete app.retryingDownload;

      DOMApplicationRegistry._saveApps(function() {
        DOMApplicationRegistry.broadcastMessage("Webapps:PackageEvent",
                                                { type: "applied",
                                                  manifestURL: app.manifestURL,
                                                  app: app,
                                                  manifest: aData });
        
        PermissionsInstaller.installPermissions({ manifest: aData,
                                                  origin: app.origin,
                                                  manifestURL: app.manifestURL },
                                                true);
      });
    }).bind(this));
  },

  startOfflineCacheDownload: function startOfflineCacheDownload(aManifest, aApp,
                                                                aProfileDir,
                                                                aOfflineCacheObserver,
                                                                aIsUpdate) {
    if (!aManifest.appcache_path) {
      return;
    }

    
    
    let appcacheURI = Services.io.newURI(aManifest.fullAppcachePath(),
                                         null, null);
    let docURI = Services.io.newURI(aManifest.fullLaunchPath(), null, null);

    
    
    
    if (aIsUpdate) {
      aApp.installState = "updating";
    }

    
    
    aApp.downloading = true;
    let cacheUpdate = aProfileDir
      ? updateSvc.scheduleCustomProfileUpdate(appcacheURI, docURI, aProfileDir)
      : updateSvc.scheduleAppUpdate(appcacheURI, docURI, aApp.localId, false);

    
    aApp.progress = 0;

    
    
    let download = {
      cacheUpdate: cacheUpdate,
      appId: this._appIdForManifestURL(aApp.manifestURL),
      previousState: aIsUpdate ? "installed" : "pending"
    };
    AppDownloadManager.add(aApp.manifestURL, download);

    cacheUpdate.addObserver(new AppcacheObserver(aApp), false);
    if (aOfflineCacheObserver) {
      cacheUpdate.addObserver(aOfflineCacheObserver, false);
    }
  },

  
  computeFileHash: function computeFileHash(aFile, aCallback) {
    Cu.import("resource://gre/modules/osfile.jsm");
    const CHUNK_SIZE = 16384;

    
    function toHexString(charCode) {
      return ("0" + charCode.toString(16)).slice(-2);
    }

    let hasher = Cc["@mozilla.org/security/hash;1"]
                   .createInstance(Ci.nsICryptoHash);
    
    hasher.init(hasher.MD5);

    OS.File.open(aFile.path, { read: true }).then(
      function opened(file) {
        let readChunk = function readChunk() {
          file.read(CHUNK_SIZE).then(
            function readSuccess(array) {
              hasher.update(array, array.length);
              if (array.length == CHUNK_SIZE) {
                readChunk();
              } else {
                
                let hash = hasher.finish(false);
                
                aCallback([toHexString(hash.charCodeAt(i)) for (i in hash)]
                          .join(""));
              }
            },
            function readError() {
              debug("Error reading " + aFile.path);
              aCallback(null);
            }
          );
        }

        readChunk();
      },
      function openError() {
        debug("Error opening " + aFile.path);
        aCallback(null);
      }
    );
  },

  
  computeManifestHash: function(aManifest) {
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                      .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";
    let result = {};
    
    let data = converter.convertToByteArray(JSON.stringify(aManifest), result);

    let hasher = Cc["@mozilla.org/security/hash;1"]
                   .createInstance(Ci.nsICryptoHash);
    hasher.init(hasher.MD5);
    hasher.update(data, data.length);
    
    let hash = hasher.finish(false);

    function toHexString(charCode) {
      return ("0" + charCode.toString(16)).slice(-2);
    }

    
    return [toHexString(hash.charCodeAt(i)) for (i in hash)].join("");
  },

  checkForUpdate: function(aData, aMm) {
    debug("checkForUpdate for " + aData.manifestURL);

    function sendError(aError) {
      aData.error = aError;
      aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:KO", aData);
    }

    let id = this._appIdForManifestURL(aData.manifestURL);
    let app = this.webapps[id];

    if (!app) {
      sendError("NO_SUCH_APP");
      return;
    }

    
    
    if (app.downloading) {
      sendError("APP_IS_DOWNLOADING");
      return;
    }

    function updatePackagedApp(aManifest) {
      debug("updatePackagedApp");

      
      
      if (app.manifestURL.startsWith("app://")) {
        aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:KO", aData);
        return;
      }

      
      let dir = FileUtils.getDir(DIRECTORY_NAME, ["webapps", id], true, true);
      let manFile = dir.clone();
      manFile.append("staged-update.webapp");
      this._writeFile(manFile, JSON.stringify(aManifest), function() { });

      let manifest = new ManifestHelper(aManifest, app.manifestURL);
      
      
      app.downloadAvailable = true;
      app.downloadSize = manifest.size;
      aData.event = "downloadavailable";
      aData.app = {
        downloadAvailable: true,
        downloadSize: manifest.size,
        updateManifest: aManifest
      }
      DOMApplicationRegistry._saveApps(function() {
        DOMApplicationRegistry.broadcastMessage("Webapps:CheckForUpdate:Return:OK",
                                                aData);
        delete aData.app.updateManifest;
      });
    }

    
    
    
    
    
    
    function updateHostedApp(aOldManifest, aNewManifest) {
      debug("updateHostedApp " + aData.manifestURL);

      
      if (id in this._manifestCache) {
        delete this._manifestCache[id];
      }

      let manifest;
      if (aNewManifest) {
        
        this.notifyAppsRegistryStart();
#ifdef MOZ_SYS_MSG
        this._unregisterActivities(aOldManifest, app);
        this._registerSystemMessages(aNewManifest, app);
        this._registerActivities(aNewManifest, app, true);
#else
        
        this.notifyAppsRegistryReady();
#endif
        
        let dir = FileUtils.getDir(DIRECTORY_NAME, ["webapps", id], true, true);
        let manFile = dir.clone();
        manFile.append("manifest.webapp");
        this._writeFile(manFile, JSON.stringify(aNewManifest), function() { });
        manifest = new ManifestHelper(aNewManifest, app.origin);
      } else {
        manifest = new ManifestHelper(aOldManifest, app.origin);
      }

      app.installState = "installed";
      app.downloading = false;
      app.downloadSize = 0;
      app.readyToApplyDownload = false;
      app.downloadAvailable = !!manifest.appcache_path;

      app.name = manifest.name;
      app.csp = manifest.csp || "";
      app.updateTime = Date.now();

      
      this.webapps[id] = app;

      this._saveApps(function() {
        let reg = DOMApplicationRegistry;
        aData.app = app;
        app.manifest = aNewManifest || aOldManifest;
        if (!manifest.appcache_path) {
          aData.event = "downloadapplied";
          reg.broadcastMessage("Webapps:CheckForUpdate:Return:OK", aData);
        } else {
          
          
          let updateObserver = {
            observe: function(aSubject, aTopic, aObsData) {
              debug("updateHostedApp: updateSvc.checkForUpdate return for " +
                    app.manifestURL + " - event is " + aTopic);
              aData.event =
                aTopic == "offline-cache-update-available" ? "downloadavailable"
                                                           : "downloadapplied";
              aData.app.downloadAvailable = (aData.event == "downloadavailable");
              reg._saveApps();
              reg.broadcastMessage("Webapps:CheckForUpdate:Return:OK", aData);
            }
          }
          debug("updateHostedApp: updateSvc.checkForUpdate for " +
                manifest.fullAppcachePath());
          updateSvc.checkForUpdate(Services.io.newURI(manifest.fullAppcachePath(), null, null),
                                   app.localId, false, updateObserver);
        }
        delete app.manifest;
      });

      
      PermissionsInstaller.installPermissions({ manifest: aNewManifest || aOldManifest,
                                                origin: app.origin,
                                                manifestURL: aData.manifestURL },
                                              true);
    }

    
    
    
    let onlyCheckAppCache = false;

#ifdef MOZ_WIDGET_GONK
      let appDir = FileUtils.getDir("coreAppsDir", ["webapps"], false);
      onlyCheckAppCache = (app.basePath == appDir.path);
#endif

    if (onlyCheckAppCache) {
      
      if (app.origin.startsWith("app://")) {
        aData.error = "NOT_UPDATABLE";
        aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:KO", aData);
        return;
      }

      
      this._readManifests([{ id: id }], function(aResult) {
        let manifest = aResult[0].manifest;
        if (!manifest.appcache_path) {
          aData.error = "NOT_UPDATABLE";
          aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:KO", aData);
          return;
        }

        app.installState = "installed";
        app.downloading = false;
        app.downloadSize = 0;
        app.readyToApplyDownload = false;
        app.updateTime = Date.now();

        debug("Checking only appcache for " + aData.manifestURL);
        
        
        let updateObserver = {
          observe: function(aSubject, aTopic, aObsData) {
            debug("onlyCheckAppCache updateSvc.checkForUpdate return for " +
                  app.manifestURL + " - event is " + aTopic);
            if (aTopic == "offline-cache-update-available") {
              aData.event = "downloadavailable";
              app.downloadAvailable = true;
              aData.app = app;
              DOMApplicationRegistry.broadcastMessage("Webapps:CheckForUpdate:Return:OK",
                                                      aData);
            } else {
              aData.error = "NOT_UPDATABLE";
              aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:KO", aData);
            }
          }
        }
        let helper = new ManifestHelper(manifest);
        debug("onlyCheckAppCache - launch updateSvc.checkForUpdate for " +
              helper.fullAppcachePath());
        updateSvc.checkForUpdate(Services.io.newURI(helper.fullAppcachePath(), null, null),
                                 app.localId, false, updateObserver);
      });
      return;
    }

    
    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                .createInstance(Ci.nsIXMLHttpRequest);
    xhr.open("GET", aData.manifestURL, true);
    xhr.channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
    xhr.responseType = "json";
    if (app.etag) {
      debug("adding manifest etag:" + app.etag);
      xhr.setRequestHeader("If-None-Match", app.etag);
    }
    xhr.channel.notificationCallbacks =
      this.createLoadContext(app.installerAppId, app.installerIsBrowser);

    xhr.addEventListener("load", (function() {
      debug("Got http status=" + xhr.status + " for " + aData.manifestURL);
      let oldHash = app.manifestHash;
      let isPackage = app.origin.startsWith("app://");

      if (xhr.status == 200) {
        let manifest = xhr.response;
        if (manifest == null) {
          sendError("MANIFEST_PARSE_ERROR");
          return;
        }

        if (!AppsUtils.checkManifest(manifest, app)) {
          sendError("INVALID_MANIFEST");
          return;
        } else if (!AppsUtils.checkInstallAllowed(manifest, app.installOrigin)) {
          sendError("INSTALL_FROM_DENIED");
          return;
        } else {
          let hash = this.computeManifestHash(manifest);
          debug("Manifest hash = " + hash);
          if (isPackage) {
            if (!app.staged) {
              app.staged = { };
            }
            app.staged.manifestHash = hash;
            app.staged.etag = xhr.getResponseHeader("Etag");
          } else {
            app.manifestHash = hash;
            app.etag = xhr.getResponseHeader("Etag");
          }

          app.lastCheckedUpdate = Date.now();
          if (isPackage) {
            if (oldHash != hash) {
              updatePackagedApp.call(this, manifest);
            } else {
              
              
              aData.event = app.downloadAvailable ? "downloadavailable"
                                                  : "downloadapplied";
              aData.app = {
                lastCheckedUpdate: app.lastCheckedUpdate
              }
              aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:OK", aData);
              this._saveApps();
            }
          } else {
            this._readManifests([{ id: id }], (function(aResult) {
              
              
              updateHostedApp.call(this, aResult[0].manifest,
                                   oldHash == hash ? null : manifest);
            }).bind(this));
          }
        }
      } else if (xhr.status == 304) {
        
        if (isPackage) {
          
          
          app.lastCheckedUpdate = Date.now();
          aData.event = app.downloadAvailable ? "downloadavailable"
                                              : "downloadapplied";
          aData.app = {
            lastCheckedUpdate: app.lastCheckedUpdate
          }
          aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:OK", aData);
          this._saveApps();
        } else {
          
          
          this._readManifests([{ id: id }], (function(aResult) {
            updateHostedApp.call(this, aResult[0].manifest, null);
          }).bind(this));
        }
      } else {
        sendError("MANIFEST_URL_ERROR");
      }
    }).bind(this), false);

    xhr.addEventListener("error", (function() {
      sendError("NETWORK_ERROR");
    }).bind(this), false);

    debug("Checking manifest at " + aData.manifestURL);
    xhr.send(null);
  },

  
  createLoadContext: function createLoadContext(aAppId, aIsBrowser) {
    return {
       associatedWindow: null,
       topWindow : null,
       appId: aAppId,
       isInBrowserElement: aIsBrowser,
       usePrivateBrowsing: false,
       isContent: false,

       isAppOfType: function(appType) {
         throw Cr.NS_ERROR_NOT_IMPLEMENTED;
       },

       QueryInterface: XPCOMUtils.generateQI([Ci.nsILoadContext,
                                              Ci.nsIInterfaceRequestor,
                                              Ci.nsISupports]),
       getInterface: function(iid) {
         if (iid.equals(Ci.nsILoadContext))
           return this;
         throw Cr.NS_ERROR_NO_INTERFACE;
       }
     }
  },

  
  
  doInstall: function doInstall(aData, aMm) {
    let app = aData.app;

    let sendError = function sendError(aError) {
      aData.error = aError;
      aMm.sendAsyncMessage("Webapps:Install:Return:KO", aData);
      Cu.reportError("Error installing app from: " + app.installOrigin +
                     ": " + aError);
    }.bind(this);

    
    
    if (this._appId(app.origin) !== null) {
      sendError("REINSTALL_FORBIDDEN");
      return;
    }

    
    
    function checkAppStatus(aManifest) {
      let manifestStatus = aManifest.type || "web";
      return manifestStatus === "web";
    }

    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                .createInstance(Ci.nsIXMLHttpRequest);
    xhr.open("GET", app.manifestURL, true);
    xhr.channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
    xhr.channel.notificationCallbacks = this.createLoadContext(aData.appId,
                                                               aData.isBrowser);
    xhr.responseType = "json";

    xhr.addEventListener("load", (function() {
      if (xhr.status == 200) {
        if (!AppsUtils.checkManifestContentType(app.installOrigin, app.origin,
                                                xhr.getResponseHeader("content-type"))) {
          sendError("INVALID_MANIFEST");
          return;
        }

        app.manifest = xhr.response;
        if (!app.manifest) {
          sendError("MANIFEST_PARSE_ERROR");
          return;
        }

        if (!AppsUtils.checkManifest(app.manifest, app)) {
          sendError("INVALID_MANIFEST");
        } else if (!AppsUtils.checkInstallAllowed(app.manifest, app.installOrigin)) {
          sendError("INSTALL_FROM_DENIED");
        } else if (!checkAppStatus(app.manifest)) {
          sendError("INVALID_SECURITY_LEVEL");
        } else {
          app.etag = xhr.getResponseHeader("Etag");
          app.manifestHash = this.computeManifestHash(app.manifest);
          
          
          let prefName = "dom.mozApps.auto_confirm_install";
          if (Services.prefs.prefHasUserValue(prefName) &&
              Services.prefs.getBoolPref(prefName)) {
            this.confirmInstall(aData);
          } else {
            Services.obs.notifyObservers(aMm, "webapps-ask-install",
                                         JSON.stringify(aData));
          }
        }
      } else {
        sendError("MANIFEST_URL_ERROR");
      }
    }).bind(this), false);

    xhr.addEventListener("error", (function() {
      sendError("NETWORK_ERROR");
    }).bind(this), false);

    xhr.send(null);
  },

  doInstallPackage: function doInstallPackage(aData, aMm) {
    let app = aData.app;

    let sendError = function sendError(aError) {
      aData.error = aError;
      aMm.sendAsyncMessage("Webapps:Install:Return:KO", aData);
      Cu.reportError("Error installing packaged app from: " +
                     app.installOrigin + ": " + aError);
    }.bind(this);

    
    
    if (this.getAppLocalIdByManifestURL(app.manifestURL) !==
        Ci.nsIScriptSecurityManager.NO_APP_ID ||
        this._appId(app.origin) !== null) {
      sendError("REINSTALL_FORBIDDEN");
      return;
    }

    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                .createInstance(Ci.nsIXMLHttpRequest);
    xhr.open("GET", app.manifestURL, true);
    xhr.channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
    xhr.channel.notificationCallbacks = this.createLoadContext(aData.appId,
                                                               aData.isBrowser);
    xhr.responseType = "json";

    xhr.addEventListener("load", (function() {
      if (xhr.status == 200) {
        if (!AppsUtils.checkManifestContentType(app.installOrigin, app.origin,
                                                xhr.getResponseHeader("content-type"))) {
          sendError("INVALID_MANIFEST");
          return;
        }

        let manifest = app.updateManifest = xhr.response;
        if (!manifest) {
          sendError("MANIFEST_PARSE_ERROR");
          return;
        }

        if (!(AppsUtils.checkManifest(manifest, app) &&
              manifest.package_path)) {
          sendError("INVALID_MANIFEST");
        } else if (!AppsUtils.checkInstallAllowed(manifest, app.installOrigin)) {
          sendError("INSTALL_FROM_DENIED");
        } else {
          app.etag = xhr.getResponseHeader("Etag");
          app.manifestHash = this.computeManifestHash(manifest);
          debug("at install package got app etag=" + app.etag);
          Services.obs.notifyObservers(aMm, "webapps-ask-install",
                                       JSON.stringify(aData));
        }
      }
      else {
        sendError("MANIFEST_URL_ERROR");
      }
    }).bind(this), false);

    xhr.addEventListener("error", (function() {
      sendError("NETWORK_ERROR");
    }).bind(this), false);

    xhr.send(null);
  },

  denyInstall: function(aData) {
    let packageId = aData.app.packageId;
    if (packageId) {
      let dir = FileUtils.getDir("TmpD", ["webapps", packageId],
                                 true, true);
      try {
        dir.remove(true);
      } catch(e) {
      }
    }
    aData.mm.sendAsyncMessage("Webapps:Install:Return:KO", aData);
  },

  
  
  
  queuedDownload: {},

  onInstallSuccessAck: function onInstallSuccessAck(aManifestURL) {
    let download = this.queuedDownload[aManifestURL];
    if (!download) {
      return;
    }
    this.startOfflineCacheDownload(download.manifest,
                                   download.app,
                                   download.profileDir,
                                   download.offlineCacheObserver);
    delete this.queuedDownload[aManifestURL];
  },

  confirmInstall: function(aData, aFromSync, aProfileDir, aOfflineCacheObserver) {
    let isReinstall = false;
    let app = aData.app;
    app.removable = true;

    let origin = Services.io.newURI(app.origin, null, null);
    let manifestURL = origin.resolve(app.manifestURL);

    let id = app.syncId || this._appId(app.origin);
    let localId = this.getAppLocalIdByManifestURL(manifestURL);

    
    if (localId && !id) {
      id = this._appIdForManifestURL(manifestURL);
    }

    
    if (id) {
      isReinstall = true;
      let dir = this._getAppDir(id);
      try {
        dir.remove(true);
      } catch(e) {
      }
    } else {
      id = this.makeAppId();
      localId = this._nextLocalId();
    }
    app.id = id;

    let manifestName = "manifest.webapp";
    if (aData.isPackage) {
      
      app.origin = "app://" + id;

      
      
      manifestName = "update.webapp";
    }

    let appObject = AppsUtils.cloneAppObject(app);
    appObject.appStatus = app.appStatus || Ci.nsIPrincipal.APP_STATUS_INSTALLED;

    appObject.installTime = app.installTime = Date.now();
    appObject.lastUpdateCheck = app.lastUpdateCheck = Date.now();
    let appNote = JSON.stringify(appObject);
    appNote.id = id;

    appObject.id = id;
    appObject.localId = localId;
    appObject.basePath = FileUtils.getDir(DIRECTORY_NAME, ["webapps"], true, true).path;
    let dir = FileUtils.getDir(DIRECTORY_NAME, ["webapps", id], true, true);
    let manFile = dir.clone();
    manFile.append(manifestName);
    let jsonManifest = aData.isPackage ? app.updateManifest : app.manifest;
    this._writeFile(manFile, JSON.stringify(jsonManifest), function() { });

    let manifest = new ManifestHelper(jsonManifest, app.origin);

    if (manifest.appcache_path) {
      appObject.installState = "pending";
      appObject.downloadAvailable = true;
      appObject.downloading = true;
      appObject.downloadSize = 0;
      appObject.readyToApplyDownload = false;
    } else if (manifest.package_path) {
      appObject.installState = "pending";
      appObject.downloadAvailable = true;
      appObject.downloading = true;
      appObject.downloadSize = manifest.size;
      appObject.readyToApplyDownload = false;
    } else {
      appObject.installState = "installed";
      appObject.downloadAvailable = false;
      appObject.downloading = false;
      appObject.readyToApplyDownload = false;
    }

    appObject.name = manifest.name;
    appObject.csp = manifest.csp || "";

    appObject.installerAppId = aData.appId;
    appObject.installerIsBrowser = aData.isBrowser;

    this.webapps[id] = appObject;

    
    
    if (!aData.isPackage) {
      PermissionsInstaller.installPermissions({ origin: appObject.origin,
                                                manifestURL: appObject.manifestURL,
                                                manifest: jsonManifest },
                                              isReinstall, (function() {
        this.uninstall(aData, aData.mm);
      }).bind(this));
    }

    ["installState", "downloadAvailable",
     "downloading", "downloadSize", "readyToApplyDownload"].forEach(function(aProp) {
      aData.app[aProp] = appObject[aProp];
     });

    this.queuedDownload[app.manifestURL] = {
      manifest: manifest,
      app: appObject,
      profileDir: aProfileDir,
      offlineCacheObserver: aOfflineCacheObserver
    }

    if (!aFromSync)
      this._saveApps((function() {
        this.broadcastMessage("Webapps:Install:Return:OK", aData);
        Services.obs.notifyObservers(this, "webapps-sync-install", appNote);
        this.broadcastMessage("Webapps:AddApp", { id: id, app: appObject });
      }).bind(this));

    if (!aData.isPackage) {
      this.notifyAppsRegistryStart();
#ifdef MOZ_SYS_MSG
      this._registerSystemMessages(app.manifest, app);
      this._registerActivities(app.manifest, app, true);
#else
      
      this.notifyAppsRegistryReady();
#endif
    }

    if (manifest.package_path) {
      
      
      manifest = new ManifestHelper(jsonManifest, app.manifestURL);
      this.downloadPackage(manifest, appObject, false, function(aId, aManifest) {
        
        let app = DOMApplicationRegistry.webapps[id];
        let zipFile = FileUtils.getFile("TmpD", ["webapps", aId, "application.zip"], true);
        let dir = FileUtils.getDir(DIRECTORY_NAME, ["webapps", aId], true, true);
        zipFile.moveTo(dir, "application.zip");
        let tmpDir = FileUtils.getDir("TmpD", ["webapps", aId], true, true);
        try {
          tmpDir.remove(true);
        } catch(e) { }

        
        let manFile = dir.clone();
        manFile.append("manifest.webapp");
        DOMApplicationRegistry._writeFile(manFile,
                                          JSON.stringify(aManifest),
                                          function() { });
        
        app.installState = "installed";
        app.downloading = false;
        app.downloadAvailable = false;
        DOMApplicationRegistry._saveApps(function() {
          
          PermissionsInstaller.installPermissions({ manifest: aManifest,
                                                    origin: appObject.origin,
                                                    manifestURL: appObject.manifestURL },
                                                  true);
          debug("About to fire Webapps:PackageEvent 'installed'");
          DOMApplicationRegistry.broadcastMessage("Webapps:PackageEvent",
                                                  { type: "installed",
                                                    manifestURL: appObject.manifestURL,
                                                    app: app,
                                                    manifest: aManifest });
        });
      });
    }
  },

  _nextLocalId: function() {
    let id = Services.prefs.getIntPref("dom.mozApps.maxLocalId") + 1;
    Services.prefs.setIntPref("dom.mozApps.maxLocalId", id);
    return id;
  },

  _appId: function(aURI) {
    for (let id in this.webapps) {
      if (this.webapps[id].origin == aURI)
        return id;
    }
    return null;
  },

  _appIdForManifestURL: function(aURI) {
    for (let id in this.webapps) {
      if (this.webapps[id].manifestURL == aURI)
        return id;
    }
    return null;
  },

  makeAppId: function() {
    let uuidGenerator = Cc["@mozilla.org/uuid-generator;1"].getService(Ci.nsIUUIDGenerator);
    return uuidGenerator.generateUUID().toString();
  },

  _saveApps: function(aCallback) {
    this._writeFile(this.appsFile, JSON.stringify(this.webapps, null, 2), function() {
      if (aCallback)
        aCallback();
    });
  },

  



  _manifestCache: {},

  _readManifests: function(aData, aFinalCallback, aIndex) {
    if (!aData.length) {
      aFinalCallback(aData);
      return;
    }

    let index = aIndex || 0;
    let id = aData[index].id;

    
    if (id in this._manifestCache) {
      aData[index].manifest = this._manifestCache[id];
      if (index == aData.length - 1)
        aFinalCallback(aData);
      else
        this._readManifests(aData, aFinalCallback, index + 1);
      return;
    }

    
    let baseDir = (this.webapps[id].removable ? DIRECTORY_NAME : "coreAppsDir");
    let file = FileUtils.getFile(baseDir, ["webapps", id, "manifest.webapp"], true);
    if (!file.exists()) {
      file = FileUtils.getFile(baseDir, ["webapps", id, "update.webapp"], true);
    }
    if (!file.exists()) {
      file = FileUtils.getFile(baseDir, ["webapps", id, "manifest.json"], true);
    }

    this._loadJSONAsync(file, (function(aJSON) {
      aData[index].manifest = this._manifestCache[id] = aJSON;
      if (index == aData.length - 1)
        aFinalCallback(aData);
      else
        this._readManifests(aData, aFinalCallback, index + 1);
    }).bind(this));
  },

  downloadPackage: function(aManifest, aApp, aIsUpdate, aOnSuccess) {
    
    
    
    
    
    
    
    

    debug("downloadPackage " + JSON.stringify(aApp));

    let id = this._appIdForManifestURL(aApp.manifestURL);
    let app = this.webapps[id];

    let self = this;
    
    function cleanup(aError) {
      debug("Cleanup: " + aError);
      let dir = FileUtils.getDir("TmpD", ["webapps", id], true, true);
      try {
        dir.remove(true);
      } catch (e) { }

      
      
      
      if (app.isCanceling) {
        delete app.isCanceling;
        return;
      }

      let download = AppDownloadManager.get(aApp.manifestURL);
      app.downloading = false;

      
      
      
      
      app.installState = download ? download.previousState
                                  : aIsUpdate ? "installed"
                                  : "pending";

      if (app.staged) {
        delete app.staged;
      }

      self.broadcastMessage("Webapps:PackageEvent",
                            { type: "error",
                              manifestURL:  aApp.manifestURL,
                              error: aError,
                              app: app });
      self._saveApps();
      AppDownloadManager.remove(aApp.manifestURL);
    }

    function sendProgressEvent() {
      self.broadcastMessage("Webapps:PackageEvent",
                            { type: "progress",
                              manifestURL: aApp.manifestURL,
                              app: app });
    }

    function download() {
      debug("About to download " + aManifest.fullPackagePath());

      let requestChannel = NetUtil.newChannel(aManifest.fullPackagePath())
                                  .QueryInterface(Ci.nsIHttpChannel);
      requestChannel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
      if (app.packageEtag) {
        debug("Add If-None-Match header: " + app.packageEtag);
        requestChannel.setRequestHeader("If-None-Match", app.packageEtag, false);
      }

      AppDownloadManager.add(aApp.manifestURL,
        {
          channel: requestChannel,
          appId: id,
          previousState: aIsUpdate ? "installed" : "pending"
        }
      );

      let lastProgressTime = 0;

      requestChannel.notificationCallbacks = {
        QueryInterface: function notifQI(aIID) {
          if (aIID.equals(Ci.nsISupports)          ||
              aIID.equals(Ci.nsIProgressEventSink) ||
              aIID.equals(Ci.nsILoadContext))
            return this;

          throw Cr.NS_ERROR_NO_INTERFACE;
        },
        getInterface: function notifGI(aIID) {
          return this.QueryInterface(aIID);
        },
        onProgress: function notifProgress(aRequest, aContext,
                                           aProgress, aProgressMax) {
          app.progress = aProgress;
          let now = Date.now();
          if (now - lastProgressTime > MIN_PROGRESS_EVENT_DELAY) {
            debug("onProgress: " + aProgress + "/" + aProgressMax);
            sendProgressEvent();
            lastProgressTime = now;
            self._saveApps();
          }
        },
        onStatus: function notifStatus(aRequest, aContext, aStatus, aStatusArg) { },

        
        appId: app.installerAppId,
        isInBrowserElement: app.installerIsBrowser,
        usePrivateBrowsing: false,
        isContent: false,
        associatedWindow: null,
        topWindow : null,
        isAppOfType: function(appType) {
          throw Cr.NS_ERROR_NOT_IMPLEMENTED;
        }
      }

      
      app.downloading = true;
      
      
      
      app.installState = aIsUpdate ? "updating" : "pending";

      
      app.progress = 0;

      
      let zipFile = FileUtils.getFile("TmpD",
                                      ["webapps", id, "application.zip"], true);

      
      let outputStream = Cc["@mozilla.org/network/file-output-stream;1"]
                           .createInstance(Ci.nsIFileOutputStream);
      
      outputStream.init(zipFile, 0x02 | 0x08 | 0x20, parseInt("0664", 8), 0);
      let bufferedOutputStream = Cc['@mozilla.org/network/buffered-output-stream;1']
                                   .createInstance(Ci.nsIBufferedOutputStream);
      bufferedOutputStream.init(outputStream, 1024);

      
      let listener = Cc["@mozilla.org/network/simple-stream-listener;1"]
                       .createInstance(Ci.nsISimpleStreamListener);
      listener.init(bufferedOutputStream, {
        onStartRequest: function(aRequest, aContext) {
          
        },

        onStopRequest: function(aRequest, aContext, aStatusCode) {
          debug("onStopRequest " + aStatusCode);
          bufferedOutputStream.close();
          outputStream.close();

          if (!Components.isSuccessCode(aStatusCode)) {
            cleanup("NETWORK_ERROR");
            return;
          }

          
          
          let responseStatus = requestChannel.responseStatus;
          if (responseStatus >= 400 && responseStatus <= 599) {
            
            app.downloadAvailable = false;
            cleanup("NETWORK_ERROR");
            return;
          }

          self.computeFileHash(zipFile, function onHashComputed(aHash) {
            debug("packageHash=" + aHash);
            let newPackage = (responseStatus != 304) &&
                             (aHash != app.packageHash);

            if (!newPackage) {
              
              
              app.downloading = false;
              app.downloadAvailable = false;
              app.downloadSize = 0;
              app.installState = "installed";
              app.readyToApplyDownload = false;
              self.broadcastMessage("Webapps:PackageEvent", {
                                      type: "applied",
                                      manifestURL: aApp.manifestURL,
                                      app: app });
              
              self._saveApps();
              let file = FileUtils.getFile("TmpD", ["webapps", id], false);
              if (file && file.exists()) {
                file.remove(true);
              }
              return;
            }

            let certdb;
            try {
              certdb = Cc["@mozilla.org/security/x509certdb;1"]
                         .getService(Ci.nsIX509CertDB);
            } catch (e) {
              
              app.downloadAvailable = false;
              cleanup("CERTDB_ERROR");
              return;
            }

            certdb.openSignedJARFileAsync(zipFile, function(aRv, aZipReader) {
              let zipReader;
              try {
                let isSigned;
                if (Components.isSuccessCode(aRv)) {
                  isSigned = true;
                  zipReader = aZipReader;
                } else if (aRv != Cr.NS_ERROR_SIGNED_JAR_NOT_SIGNED) {
                  throw "INVALID_SIGNATURE";
                } else {
                  isSigned = false;
                  zipReader = Cc["@mozilla.org/libjar/zip-reader;1"]
                                .createInstance(Ci.nsIZipReader);
                  zipReader.open(zipFile);
                }

                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                
                let signedAppOriginsStr =
                  Services.prefs.getCharPref(
                    "dom.mozApps.signed_apps_installable_from");
                let isSignedAppOrigin
                  = signedAppOriginsStr.split(",").indexOf(aApp.installOrigin) > -1;
                if (!isSigned && isSignedAppOrigin) {
                  
                  
                  throw "INVALID_SIGNATURE";
                } else if (isSigned && !isSignedAppOrigin) {
                  
                  
                  
                  
                  
                  throw "INSTALL_FROM_DENIED";
                }

                if (!zipReader.hasEntry("manifest.webapp")) {
                  throw "MISSING_MANIFEST";
                }

                let istream = zipReader.getInputStream("manifest.webapp");

                
                let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                                  .createInstance(Ci.nsIScriptableUnicodeConverter);
                converter.charset = "UTF-8";

                let manifest = JSON.parse(converter.ConvertToUnicode(NetUtil.readInputStreamToString(istream,
                                                                     istream.available()) || ""));

                
                
                
                if (!AppsUtils.checkManifest(manifest, app)) {
                  throw "INVALID_MANIFEST";
                }

                if (!AppsUtils.compareManifests(manifest,
                                                aManifest._manifest)) {
                  throw "MANIFEST_MISMATCH";
                }

                if (!AppsUtils.checkInstallAllowed(manifest, aApp.installOrigin)) {
                  throw "INSTALL_FROM_DENIED";
                }

                let maxStatus = isSigned ? Ci.nsIPrincipal.APP_STATUS_PRIVILEGED
                                         : Ci.nsIPrincipal.APP_STATUS_INSTALLED;

                if (AppsUtils.getAppManifestStatus(manifest) > maxStatus) {
                  throw "INVALID_SECURITY_LEVEL";
                }
                app.appStatus = AppsUtils.getAppManifestStatus(manifest);

                
                if (aIsUpdate) {
                  if (!app.staged) {
                    app.staged = { };
                  }
                  try {
                    app.staged.packageEtag =
                      requestChannel.getResponseHeader("Etag");
                  } catch(e) { }
                  app.staged.packageHash = aHash;
                  app.staged.appStatus =
                    AppsUtils.getAppManifestStatus(manifest);
                } else {
                  try {
                    app.packageEtag = requestChannel.getResponseHeader("Etag");
                  } catch(e) { }
                  app.packageHash = aHash;
                  app.appStatus = AppsUtils.getAppManifestStatus(manifest);
                }

                if (aOnSuccess) {
                  aOnSuccess(id, manifest);
                }
              } catch (e) {
                
                
                app.downloadAvailable = false;
                if (typeof e == 'object') {
                  Cu.reportError("Error while reading package:" + e);
                  cleanup("INVALID_PACKAGE");
                } else {
                  cleanup(e);
                }
              } finally {
                AppDownloadManager.remove(aApp.manifestURL);
                if (zipReader)
                  zipReader.close();
              }
            });
          });
        }
      });

      requestChannel.asyncOpen(listener, null);

      
      sendProgressEvent();
    };

    let deviceStorage = Services.wm.getMostRecentWindow("navigator:browser")
                                .navigator.getDeviceStorage("apps");
    let req = deviceStorage.freeSpace();
    req.onsuccess = req.onerror = function statResult(e) {
      
      
      if (!e.target.result) {
        download();
        return;
      }

      let freeBytes = e.target.result;
      if (freeBytes) {
        debug("Free storage: " + freeBytes + ". Download size: " +
              aApp.downloadSize);
        if (freeBytes <=
            aApp.downloadSize + AppDownloadManager.MIN_REMAINING_FREESPACE) {
          cleanup("INSUFFICIENT_STORAGE");
          return;
        }
      }
      download();
    }
  },

  uninstall: function(aData, aMm) {
    debug("uninstall " + aData.origin);
    for (let id in this.webapps) {
      let app = this.webapps[id];
      if (app.origin != aData.origin) {
        continue;
      }

      dump("-- webapps.js uninstall " + app.manifestURL + "\n");

      if (!app.removable) {
        debug("Error: cannot unintall a non-removable app.");
        break;
      }

      
      
      this.cancelDownload(app.manifestURL);

      
      if (id in this._manifestCache) {
        delete this._manifestCache[id];
      }

      
      this._clearPrivateData(app.localId, false);

      
      Services.obs.notifyObservers(aMm, "webapps-uninstall", JSON.stringify(aData));

      let appNote = JSON.stringify(AppsUtils.cloneAppObject(app));
      appNote.id = id;

#ifdef MOZ_SYS_MSG
      this._readManifests([{ id: id }], (function unregisterManifest(aResult) {
        this._unregisterActivities(aResult[0].manifest, app);
      }).bind(this));
#endif

      let dir = this._getAppDir(id);
      try {
        dir.remove(true);
      } catch (e) {}

      delete this.webapps[id];

      this._saveApps((function() {
        aData.manifestURL = app.manifestURL;
        this.broadcastMessage("Webapps:Uninstall:Broadcast:Return:OK", aData);
        aMm.sendAsyncMessage("Webapps:Uninstall:Return:OK", aData);
        Services.obs.notifyObservers(this, "webapps-sync-uninstall", appNote);
        this.broadcastMessage("Webapps:RemoveApp", { id: id });
      }).bind(this));

      return;
    }

    
    
    
    aMm.sendAsyncMessage("Webapps:Uninstall:Return:KO", aData);
  },

  getSelf: function(aData, aMm) {
    aData.apps = [];

    if (aData.appId == Ci.nsIScriptSecurityManager.NO_APP_ID ||
        aData.appId == Ci.nsIScriptSecurityManager.UNKNOWN_APP_ID) {
      aMm.sendAsyncMessage("Webapps:GetSelf:Return:OK", aData);
      return;
    }

    let tmp = [];

    for (let id in this.webapps) {
      if (this.webapps[id].origin == aData.origin &&
          this.webapps[id].localId == aData.appId &&
          this._isLaunchable(this.webapps[id].origin)) {
        let app = AppsUtils.cloneAppObject(this.webapps[id]);
        aData.apps.push(app);
        tmp.push({ id: id });
        break;
      }
    }

    if (!aData.apps.length) {
      aMm.sendAsyncMessage("Webapps:GetSelf:Return:OK", aData);
      return;
    }

    this._readManifests(tmp, (function(aResult) {
      for (let i = 0; i < aResult.length; i++)
        aData.apps[i].manifest = aResult[i].manifest;
      aMm.sendAsyncMessage("Webapps:GetSelf:Return:OK", aData);
    }).bind(this));
  },

  checkInstalled: function(aData, aMm) {
    aData.app = null;
    let tmp = [];

    for (let appId in this.webapps) {
      if (this.webapps[appId].manifestURL == aData.manifestURL) {
        aData.app = AppsUtils.cloneAppObject(this.webapps[appId]);
        tmp.push({ id: appId });
        break;
      }
    }

    this._readManifests(tmp, (function(aResult) {
      for (let i = 0; i < aResult.length; i++) {
        aData.app.manifest = aResult[i].manifest;
        break;
      }
      aMm.sendAsyncMessage("Webapps:CheckInstalled:Return:OK", aData);
    }).bind(this));
  },

  getInstalled: function(aData, aMm) {
    aData.apps = [];
    let tmp = [];

    for (let id in this.webapps) {
      if (this.webapps[id].installOrigin == aData.origin &&
          this._isLaunchable(this.webapps[id].origin)) {
        aData.apps.push(AppsUtils.cloneAppObject(this.webapps[id]));
        tmp.push({ id: id });
      }
    }

    this._readManifests(tmp, (function(aResult) {
      for (let i = 0; i < aResult.length; i++)
        aData.apps[i].manifest = aResult[i].manifest;
      aMm.sendAsyncMessage("Webapps:GetInstalled:Return:OK", aData);
    }).bind(this));
  },

  getNotInstalled: function(aData, aMm) {
    aData.apps = [];
    let tmp = [];

    for (let id in this.webapps) {
      if (!this._isLaunchable(this.webapps[id].origin)) {
        aData.apps.push(AppsUtils.cloneAppObject(this.webapps[id]));
        tmp.push({ id: id });
      }
    }

    this._readManifests(tmp, (function(aResult) {
      for (let i = 0; i < aResult.length; i++)
        aData.apps[i].manifest = aResult[i].manifest;
      aMm.sendAsyncMessage("Webapps:GetNotInstalled:Return:OK", aData);
    }).bind(this));
  },

  getAll: function(aData, aMm) {
    aData.apps = [];
    let tmp = [];

    for (let id in this.webapps) {
      let app = AppsUtils.cloneAppObject(this.webapps[id]);
      if (!this._isLaunchable(app.origin))
        continue;

      aData.apps.push(app);
      tmp.push({ id: id });
    }

    this._readManifests(tmp, (function(aResult) {
      for (let i = 0; i < aResult.length; i++)
        aData.apps[i].manifest = aResult[i].manifest;
      aMm.sendAsyncMessage("Webapps:GetAll:Return:OK", aData);
    }).bind(this));
  },

  getManifestFor: function(aOrigin, aCallback) {
    if (!aCallback)
      return;

    let id = this._appId(aOrigin);
    let app = this.webapps[id];
    if (!id || (app.installState == "pending" && !app.retryingDownload)) {
      aCallback(null);
      return;
    }

    this._readManifests([{ id: id }], function(aResult) {
      aCallback(aResult[0].manifest);
    });
  },

  
  itemExists: function(aId) {
    return !!this.webapps[aId];
  },

  getAppById: function(aId) {
    if (!this.webapps[aId])
      return null;

    let app = AppsUtils.cloneAppObject(this.webapps[aId]);
    return app;
  },

  getAppByManifestURL: function(aManifestURL) {
    return AppsUtils.getAppByManifestURL(this.webapps, aManifestURL);
  },

  getCSPByLocalId: function(aLocalId) {
    debug("getCSPByLocalId:" + aLocalId);
    return AppsUtils.getCSPByLocalId(this.webapps, aLocalId);
  },

  getAppByLocalId: function(aLocalId) {
    return AppsUtils.getAppByLocalId(this.webapps, aLocalId);
  },

  getManifestURLByLocalId: function(aLocalId) {
    return AppsUtils.getManifestURLByLocalId(this.webapps, aLocalId);
  },

  getAppLocalIdByManifestURL: function(aManifestURL) {
    return AppsUtils.getAppLocalIdByManifestURL(this.webapps, aManifestURL);
  },

  getAppFromObserverMessage: function(aMessage) {
    return AppsUtils.getAppFromObserverMessage(this.webapps, aMessage);
  },

  getCoreAppsBasePath: function() {
    return FileUtils.getDir("coreAppsDir", ["webapps"], false).path;
  },

  getWebAppsBasePath: function getWebAppsBasePath() {
    return FileUtils.getDir(DIRECTORY_NAME, ["webapps"], false).path;
  },

  getAllWithoutManifests: function(aCallback) {
    let result = {};
    for (let id in this.webapps) {
      let app = AppsUtils.cloneAppObject(this.webapps[id]);
      result[id] = app;
    }
    aCallback(result);
  },

  updateApps: function(aRecords, aCallback) {
    for (let i = 0; i < aRecords.length; i++) {
      let record = aRecords[i];
      if (record.hidden) {
        if (!this.webapps[record.id] || !this.webapps[record.id].removable)
          continue;

        
        if (record.id in this._manifestCache) {
          delete this._manifestCache[record.id];
        }

        let origin = this.webapps[record.id].origin;
        let manifestURL = this.webapps[record.id].manifestURL;
        delete this.webapps[record.id];
        let dir = this._getAppDir(record.id);
        try {
          dir.remove(true);
        } catch (e) {
        }
        this.broadcastMessage("Webapps:Uninstall:Broadcast:Return:OK",
                              { origin: origin, manifestURL: manifestURL });
      } else {
        if (this.webapps[record.id]) {
          this.webapps[record.id] = record.value;
          delete this.webapps[record.id].manifest;
        } else {
          let data = { app: record.value };
          this.confirmInstall(data, true);
          this.broadcastMessage("Webapps:Install:Return:OK", data);
        }
      }
    }
    this._saveApps(aCallback);
  },

  getAllIDs: function() {
    let apps = {};
    for (let id in this.webapps) {
      
      if (this.webapps[id].origin.indexOf("http") == 0)
        apps[id] = true;
    }
    return apps;
  },

  wipe: function(aCallback) {
    let ids = this.getAllIDs();
    for (let id in ids) {
      if (!this.webapps[id].removable) {
        continue;
      }

      delete this.webapps[id];
      let dir = this._getAppDir(id);
      try {
        dir.remove(true);
      } catch (e) {
      }
    }

    
    this._manifestCache = { };

    this._saveApps(aCallback);
  },

  _isLaunchable: function(aOrigin) {
    if (this.allAppsLaunchable)
      return true;

#ifdef XP_WIN
    let uninstallKey = Cc["@mozilla.org/windows-registry-key;1"]
                         .createInstance(Ci.nsIWindowsRegKey);
    try {
      uninstallKey.open(uninstallKey.ROOT_KEY_CURRENT_USER,
                        "SOFTWARE\\Microsoft\\Windows\\" +
                        "CurrentVersion\\Uninstall\\" +
                        aOrigin,
                        uninstallKey.ACCESS_READ);
      uninstallKey.close();
      return true;
    } catch (ex) {
      return false;
    }
#elifdef XP_MACOSX
    let mwaUtils = Cc["@mozilla.org/widget/mac-web-app-utils;1"]
                     .createInstance(Ci.nsIMacWebAppUtils);

    return !!mwaUtils.pathForAppWithIdentifier(aOrigin);
#elifdef XP_UNIX
    let env = Cc["@mozilla.org/process/environment;1"]
                .getService(Ci.nsIEnvironment);
    let xdg_data_home_env;
    try {
      xdg_data_home_env = env.get("XDG_DATA_HOME");
    } catch(ex) {
    }

    let desktopINI;
    if (xdg_data_home_env) {
      desktopINI = new FileUtils.File(xdg_data_home_env);
    } else {
      desktopINI = FileUtils.getFile("Home", [".local", "share"]);
    }
    desktopINI.append("applications");

    let origin = Services.io.newURI(aOrigin, null, null);
    let uniqueName = origin.scheme + ";" +
                     origin.host +
                     (origin.port != -1 ? ";" + origin.port : "");

    desktopINI.append("owa-" + uniqueName + ".desktop");

    return desktopINI.exists();
#else
    return true;
#endif

  },

  _notifyCategoryAndObservers: function(subject, topic, data) {
    const serviceMarker = "service,";

    
    let cm =
      Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager);
    let enumerator = cm.enumerateCategory(topic);

    let observers = [];

    while (enumerator.hasMoreElements()) {
      let entry =
        enumerator.getNext().QueryInterface(Ci.nsISupportsCString).data;
      let contractID = cm.getCategoryEntry(topic, entry);

      let factoryFunction;
      if (contractID.substring(0, serviceMarker.length) == serviceMarker) {
        contractID = contractID.substring(serviceMarker.length);
        factoryFunction = "getService";
      }
      else {
        factoryFunction = "createInstance";
      }

      try {
        let handler = Cc[contractID][factoryFunction]();
        if (handler) {
          let observer = handler.QueryInterface(Ci.nsIObserver);
          observers.push(observer);
        }
      } catch(e) { }
    }

    
    enumerator = Services.obs.enumerateObservers(topic);
    while (enumerator.hasMoreElements()) {
      try {
        let observer = enumerator.getNext().QueryInterface(Ci.nsIObserver);
        if (observers.indexOf(observer) == -1) {
          observers.push(observer);
        }
      } catch (e) { }
    }

    observers.forEach(function (observer) {
      try {
        observer.observe(subject, topic, data);
      } catch(e) { }
    });
  },

  registerBrowserElementParentForApp: function(bep, appId) {
    let mm = bep._mm;

    
    let listener = this.receiveAppMessage.bind(this, appId);

    this.frameMessages.forEach(function(msgName) {
      mm.addMessageListener(msgName, listener);
    });
  },

  receiveAppMessage: function(appId, message) {
    switch (message.name) {
      case "Webapps:ClearBrowserData":
        this._clearPrivateData(appId, true);
        break;
    }
  },

  _clearPrivateData: function(appId, browserOnly) {
    let subject = {
      appId: appId,
      browserOnly: browserOnly,
      QueryInterface: XPCOMUtils.generateQI([Ci.mozIApplicationClearPrivateDataParams])
    };
    this._notifyCategoryAndObservers(subject, "webapps-clear-data", null);
  }
};




let AppcacheObserver = function(aApp) {
  debug("Creating AppcacheObserver for " + aApp.origin +
        " - " + aApp.installState);
  this.app = aApp;
  this.startStatus = aApp.installState;
  this.lastProgressTime = 0;
  
  this._sendProgressEvent();
};

AppcacheObserver.prototype = {
  
  _sendProgressEvent: function() {
    let app = this.app;
    DOMApplicationRegistry.broadcastMessage("Webapps:OfflineCache",
                                            { manifest: app.manifestURL,
                                              installState: app.installState,
                                              progress: app.progress });
  },

  updateStateChanged: function appObs_Update(aUpdate, aState) {
    let mustSave = false;
    let app = this.app;

    debug("Offline cache state change for " + app.origin + " : " + aState);

    var self = this;
    let setStatus = function appObs_setStatus(aStatus, aProgress) {
      debug("Offlinecache setStatus to " + aStatus + " with progress " +
          aProgress + " for " + app.origin);
      mustSave = (app.installState != aStatus);
      app.installState = aStatus;
      app.progress = aProgress;
      if (aStatus == "installed") {
        app.downloading = false;
        app.downloadAvailable = false;
      }
      self._sendProgressEvent();
    }

    let setError = function appObs_setError(aError) {
      debug("Offlinecache setError to " + aError);
      
      
      if (!app.isCanceling) {
        DOMApplicationRegistry.broadcastMessage("Webapps:OfflineCache",
                                                { manifest: app.manifestURL,
                                                  error: aError });
      } else {
        delete app.isCanceling;
      }

      app.downloading = false;
      mustSave = true;
    }

    switch (aState) {
      case Ci.nsIOfflineCacheUpdateObserver.STATE_ERROR:
        aUpdate.removeObserver(this);
        AppDownloadManager.remove(app.manifestURL);
        setError("APP_CACHE_DOWNLOAD_ERROR");
        break;
      case Ci.nsIOfflineCacheUpdateObserver.STATE_NOUPDATE:
      case Ci.nsIOfflineCacheUpdateObserver.STATE_FINISHED:
        aUpdate.removeObserver(this);
        AppDownloadManager.remove(app.manifestURL);
        setStatus("installed", aUpdate.byteProgress);
        break;
      case Ci.nsIOfflineCacheUpdateObserver.STATE_DOWNLOADING:
      case Ci.nsIOfflineCacheUpdateObserver.STATE_ITEMSTARTED:
        setStatus(this.startStatus, aUpdate.byteProgress);
        break;
      case Ci.nsIOfflineCacheUpdateObserver.STATE_ITEMPROGRESS:
        let now = Date.now();
        if (now - this.lastProgressTime > MIN_PROGRESS_EVENT_DELAY) {
          setStatus(this.startStatus, aUpdate.byteProgress);
          this.lastProgressTime = now;
        }
        break;
    }

    
    if (mustSave) {
      DOMApplicationRegistry._saveApps();
    }
  },

  applicationCacheAvailable: function appObs_CacheAvail(aApplicationCache) {
    
  }
};

DOMApplicationRegistry.init();
