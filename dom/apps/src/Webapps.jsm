



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

function debug(aMsg) {
  
}

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
  downloads: { },

  init: function() {
    this.messages = ["Webapps:Install", "Webapps:Uninstall",
                     "Webapps:GetSelf", "Webapps:CheckInstalled",
                     "Webapps:GetInstalled", "Webapps:GetNotInstalled",
                     "Webapps:Launch", "Webapps:GetAll",
                     "Webapps:InstallPackage", "Webapps:GetBasePath",
                     "Webapps:GetList", "Webapps:RegisterForMessages",
                     "Webapps:UnregisterForMessages",
                     "Webapps:CancelDownload", "Webapps:CheckForUpdate",
                     "Webapps:Download", "Webapps:ApplyDownload",
                     "child-process-shutdown"];

    this.frameMessages = ["Webapps:ClearBrowserData"];

    this.messages.forEach((function(msgName) {
      ppmm.addMessageListener(msgName, this);
    }).bind(this));

    cpmm.addMessageListener("Activities:Register:OK", this);

    Services.obs.addObserver(this, "xpcom-shutdown", false);

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
            
            if (this.webapps[id].localId === undefined) {
              this.webapps[id].localId = this._nextLocalId();
            }

            if (this.webapps[id].basePath === undefined) {
              this.webapps[id].basePath = appDir.path;
            }

            
            if (this.webapps[id].removable === undefined) {
              this.webapps[id].removable = true;
            }

            
            if (this.webapps[id].appStatus === undefined) {
              this.webapps[id].appStatus = Ci.nsIPrincipal.APP_STATUS_INSTALLED;
            }
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
        this.webapps[aResult.id].csp = manifest.csp || "";
      }, this);
    }).bind(this));

    
    this.notifyAppsRegistryReady();
#endif
  },

  updatePermissionsForApp: function updatePermissionsForApp(aId) {
    
    
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
          permMgr.RemovePermissionsForApp(localId);
          Services.cookies.removeCookiesForApp(localId, false);
          this._clearPrivateData(localId, false);
        }

        let appDir = FileUtils.getDir("coreAppsDir", ["webapps"], false);
        
        for (let id in aData) {
          
          
          if (!(id in this.webapps)) {
            this.webapps[id] = aData[id];
            this.webapps[id].basePath = appDir.path;

            
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
          this.updatePermissionsForApp(id);
          this.updateOfflineCacheForApp(id);
        }
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
        href = Services.io.newURI(manifest.resolveFromOrigin(aMessage[messageName]), null, null);
      } else {
        messageName = aMessage;
      }
      msgmgr.registerPage(messageName, href, manifestURL);
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
      if (!description.href) {
        description.href = manifest.launch_path;
      }
      description.href = manifest.resolveFromOrigin(description.href);

      if (aRunUpdate) {
        activitiesToRegister.push({ "manifest": aApp.manifestURL,
                                    "name": activity,
                                    "title": manifest.name,
                                    "icon": manifest.iconURLForSize(128),
                                    "description": description });
      }

      let launchPath = Services.io.newURI(description.href, null, null);
      let manifestURL = Services.io.newURI(aApp.manifestURL, null, null);
      msgmgr.registerPage("activity", launchPath, manifestURL);
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
         "Webapps:ApplyDownload"].indexOf(aMessage.name) != -1) {
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
        
        Services.obs.notifyObservers(mm, "webapps-ask-install", JSON.stringify(msg));
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
        if (msg.hasPrivileges)
          this.getAll(msg, mm);
        else
          mm.sendAsyncMessage("Webapps:GetAll:Return:KO", msg);
        break;
      case "Webapps:InstallPackage":
        
        Services.obs.notifyObservers(mm, "webapps-ask-install", JSON.stringify(msg));
        break;
      case "Webapps:GetBasePath":
        return this.webapps[msg.id].basePath;
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
    }
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

  cancelDownload: function cancelDownload(aManifestURL) {
    
    if (!this.downloads[aManifestURL]) {
      return;
    }
    
    let download = this.downloads[aManifestURL]
    download.channel.cancel(Cr.NS_BINDING_ABORTED);
    let app = this.webapps[download.appId];

    app.progress = 0;
    app.installState = app.previousState;
    app.downloading = false;
    app.downloadAvailable = false;
    app.downloadSize = 0;
    this._saveApps((function() {
      this.broadcastMessage("Webapps:PackageEvent",
                             { type: "canceled",
                               manifestURL:  aApp.manifestURL,
                               app: app,
                               error: "DOWNLOAD_CANCELED" });
    }).bind(this));
  },

  startDownload: function startDownload(aManifestURL) {
    debug("startDownload for " + aManifestURL);
    let id = this._appIdForManifestURL(aManifestURL);
    let app = this.webapps[id];
    if (!app) {
      debug("startDownload: No app found for " + aManifestURL);
      return;
    }

    
    let file = FileUtils.getFile(DIRECTORY_NAME,
                                 ["webapps", id, "update.webapp"], true);

    this._loadJSONAsync(file, (function(aJSON) {
      if (!aJSON) {
        debug("startDownload: No update manifest found at " + file.path + " " + aManifestURL);
        return;
      }

      let manifest = new ManifestHelper(aJSON, app.installOrigin);
      this.downloadPackage(manifest, { manifestURL: aManifestURL,
                                       origin: app.origin }, true,
        function(aId, aManifest) {
          
          
          let tmpDir = FileUtils.getDir("TmpD", ["webapps", aId], true, true);

          
          let manFile = tmpDir.clone();
          manFile.append("manifest.webapp");
          DOMApplicationRegistry._writeFile(manFile,
                                            JSON.stringify(aManifest),
                                            function() { });
          
          app.downloading = false;
          app.downloadAvailable = false;
          app.readyToApplyDownload = true;
          DOMApplicationRegistry._saveApps(function() {
            debug("About to fire Webapps:PackageEvent");
            DOMApplicationRegistry.broadcastMessage("Webapps:PackageEvent",
                                                    { type: "downloaded",
                                                      manifestURL: aManifestURL,
                                                      app: app,
                                                      manifest: aManifest });
          });
        });
    }).bind(this));
  },

  applyDownload: function applyDownload(aManifestURL) {
    debug("applyDownload for " + aManifestURL);
    let app = this.getAppByManifestURL(aManifestURL);
    if (!app || (app && !app.readyToApplyDownload)) {
      return;
    }

    let id = this._appIdForManifestURL(app.manifestURL);

    
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

    try {
      tmpDir.remove(true);
    } catch(e) { }

    
    this.getManifestFor(app.origin, (function(aData) {
      app.readyToApplyDownload = false;
      this.broadcastMessage("Webapps:PackageEvent",
                            { type: "applied",
                              manifestURL: app.manifestURL,
                              app: app,
                              manifest: aData });
      
      PermissionsInstaller.installPermissions({ manifest: aData,
                                                origin: app.origin,
                                                manifestURL: app.manifestURL },
                                              true);
    }).bind(this));
  },

  startOfflineCacheDownload: function startOfflineCacheDownload(aManifest, aApp,
                                                                aProfileDir,
                                                                aOfflineCacheObserver) {
    
    if (aManifest.appcache_path) {
      let appcacheURI = Services.io.newURI(aManifest.fullAppcachePath(), null, null);
      let updateService = Cc["@mozilla.org/offlinecacheupdate-service;1"]
                            .getService(Ci.nsIOfflineCacheUpdateService);
      let docURI = Services.io.newURI(aManifest.fullLaunchPath(), null, null);
      let cacheUpdate = aProfileDir ? updateService.scheduleCustomProfileUpdate(appcacheURI, docURI, aProfileDir)
                                    : updateService.scheduleAppUpdate(appcacheURI, docURI, aApp.localId, false);
      cacheUpdate.addObserver(new AppcacheObserver(aApp), false);
      if (aOfflineCacheObserver) {
        cacheUpdate.addObserver(aOfflineCacheObserver, false);
      }
    }
  },

  checkForUpdate: function(aData, aMm) {
    debug("checkForUpdate for " + aData.manifestURL);
    let id = this._appIdForManifestURL(aData.manifestURL);
    let app = this.webapps[id];

    if (!app) {
      aData.error = "NO_SUCH_APP";
      aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:KO", aData);
      return;
    }

    function sendError(aError) {
      aData.error = aError;
      aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:KO", aData);
    }

    function updatePackagedApp(aManifest) {
      debug("updatePackagedApp");

      
      
      if (app.manifestURL.startsWith("app://")) {
        aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:KO", aData);
        return;
      }

      let manifest = new ManifestHelper(aManifest, app.manifestURL);
      
      
      app.downloadAvailable = true;
      app.downloadSize = manifest.size;
      aData.event = "downloadavailable";
      aData.app = {
        downloadAvailable: true,
        downloadSize: manifest.size
      }
      DOMApplicationRegistry._saveApps(function() {
        aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:OK", aData);
      });
    }

    function updateHostedApp(aManifest) {
      debug("updateHostedApp");
      let id = this._appId(app.origin);

      if (id in this._manifestCache) {
        delete this._manifestCache[id];
      }

      
      this.notifyAppsRegistryStart();
#ifdef MOZ_SYS_MSG
      this._readManifests([{ id: id }], (function unregisterManifest(aResult) {
        this._unregisterActivities(aResult[0].manifest, app);
        this._registerSystemMessages(aManifest, app);
        this._registerActivities(aManifest, app, true);
      }).bind(this));
#else
      
      this.notifyAppsRegistryReady();
#endif

      
      let dir = FileUtils.getDir(DIRECTORY_NAME, ["webapps", id], true, true);
      let manFile = dir.clone();
      manFile.append("manifest.webapp");
      this._writeFile(manFile, JSON.stringify(aManifest), function() { });

      let manifest = new ManifestHelper(aManifest, app.origin);

      if (manifest.appcache_path) {
        app.installState = "updating";
        app.downloadAvailable = true;
        app.downloading = true;
        app.downloadsize = 0;
        app.readyToApplyDownload = false;
      } else {
        app.installState = "installed";
        app.downloadAvailable = false;
        app.downloading = false;
        app.readyToApplyDownload = false;
      }

      app.name = aManifest.name;
      app.csp = aManifest.csp || "";

      
      this.webapps[id] = app;

      this._saveApps(function() {
        aData.event = "downloadapplied";
        aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:OK", aData);
      });

      
      this.startOfflineCacheDownload(manifest, app);

      
      PermissionsInstaller.installPermissions({ manifest: aManifest,
                                                origin: app.origin,
                                                manifestURL: aData.manifestURL },
                                                true);
    }

    
    let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                .createInstance(Ci.nsIXMLHttpRequest);
    xhr.open("GET", aData.manifestURL, true);
    if (app.etag) {
      xhr.setRequestHeader("If-None-Match", app.etag);
    }

    xhr.addEventListener("load", (function() {
      if (xhr.status == 200) {
        let manifest;
        try {
          manifest = JSON.parse(xhr.responseText);
        } catch(e) {
          sendError("MANIFEST_PARSE_ERROR");
          return;
        }
        if (!AppsUtils.checkManifest(manifest, app.installOrigin)) {
          sendError("INVALID_MANIFEST");
        } else {
          app.etag = xhr.getResponseHeader("Etag");
          app.lastCheckedUpdate = Date.now();
          if (app.origin.startsWith("app://")) {
            updatePackagedApp(manifest);
          } else {
            updateHostedApp.call(this, manifest);
          }
        }
        this._saveApps();
      } else if (xhr.status == 304) {
        
        app.lastCheckedUpdate = Date.now();
        aData.event = "downloadapplied";
        aData.app = {
          lastCheckedUpdate: app.lastCheckedUpdate
        }
        aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:OK", aData);
        this._saveApps();
      } else {
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

  confirmInstall: function(aData, aFromSync, aProfileDir, aOfflineCacheObserver) {
    let isReinstall = false;
    let app = aData.app;
    app.removable = true;

    let origin = Services.io.newURI(app.origin, null, null);
    let manifestURL = origin.resolve(app.manifestURL);

    let id = app.syncId || this._appId(app.origin);
    let localId = this.getAppLocalIdByManifestURL(manifestURL);

    
    if (id) {
      isReinstall = true;
      let dir = this._getAppDir(id);
      try {
        dir.remove(true);
      } catch(e) {
      }
    } else {
      id = this.makeAppId();
      app.id = id;
      localId = this._nextLocalId();
    }

    let manifestName = "manifest.webapp";
    if (aData.isPackage) {
      
      app.origin = "app://" + id;

      
      
      manifestName = "update.webapp";
    }

    let appObject = AppsUtils.cloneAppObject(app);
    appObject.appStatus = app.appStatus || Ci.nsIPrincipal.APP_STATUS_INSTALLED;
    
    if (!aData.isPackage &&
        Services.prefs.getBoolPref("dom.mozApps.dev_mode")) {
      appObject.appStatus = AppsUtils.getAppManifestStatus(app.manifest);
    }

    appObject.installTime = app.installTime = Date.now();
    appObject.lastUpdateCheck = app.lastUpdateCheck = Date.now();
    let appNote = JSON.stringify(appObject);
    appNote.id = id;

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

    this.startOfflineCacheDownload(manifest, appObject, aProfileDir, aOfflineCacheObserver);
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
    
    
    
    
    
    
    

    debug(JSON.stringify(aApp));

    let id = this._appIdForManifestURL(aApp.manifestURL);
    let app = this.webapps[id];

    
    function cleanup(aError) {
      debug("Cleanup: " + aError);
      let dir = FileUtils.getDir("TmpD", ["webapps", id], true, true);
      try {
        dir.remove(true);
      } catch (e) { }
        this.broadcastMessage("Webapps:PackageEvent",
                              { type: "error",
                                manifestURL:  aApp.manifestURL,
                                error: aError});
    }

    function getInferedStatus() {
      
      return Ci.nsIPrincipal.APP_STATUS_INSTALLED;
    }

    function getAppStatus(aManifest) {
      let manifestStatus = AppsUtils.getAppManifestStatus(aManifest);
      let inferedStatus = getInferedStatus();

      return (Services.prefs.getBoolPref("dom.mozApps.dev_mode") ? manifestStatus
                                                                : inferedStatus);
    }
    
    
    function checkAppStatus(aManifest) {
      if (Services.prefs.getBoolPref("dom.mozApps.dev_mode")) {
        return true;
      }

      return (AppsUtils.getAppManifestStatus(aManifest) <= getInferedStatus());
    }

    debug("About to download " + aManifest.fullPackagePath());

    let requestChannel = NetUtil.newChannel(aManifest.fullPackagePath())
                                .QueryInterface(Ci.nsIHttpChannel);
    this.downloads[aApp.manifestURL] =
      { channel:requestChannel,
        appId: id,
        previousState: aIsUpdate ? "installed" : "pending"
      };
    requestChannel.notificationCallbacks = {
      QueryInterface: function notifQI(aIID) {
        if (aIID.equals(Ci.nsISupports)          ||
            aIID.equals(Ci.nsIProgressEventSink))
          return this;

        throw Cr.NS_ERROR_NO_INTERFACE;
      },
      getInterface: function notifGI(aIID) {
        return this.QueryInterface(aIID);
      },
      onProgress: function notifProgress(aRequest, aContext,
                                         aProgress, aProgressMax) {
        debug("onProgress: " + aProgress + "/" + aProgressMax);
        app.progress = aProgress;
        DOMApplicationRegistry.broadcastMessage("Webapps:PackageEvent",
                                                { type: "progress",
                                                  manifestURL: aApp.manifestURL,
                                                  progress: aProgress });
      },
      onStatus: function notifStatus(aRequest, aContext, aStatus, aStatusArg) { }
    }

    NetUtil.asyncFetch(requestChannel,
    function(aInput, aResult, aRequest) {
      if (!Components.isSuccessCode(aResult)) {
        
        cleanup("NETWORK_ERROR");
        return;
      }
      
      let zipFile = FileUtils.getFile("TmpD",
                                      ["webapps", id, "application.zip"], true);
      let ostream = FileUtils.openSafeFileOutputStream(zipFile);
      NetUtil.asyncCopy(aInput, ostream, function (aResult) {
        if (!Components.isSuccessCode(aResult)) {
          
          cleanup("DOWNLOAD_ERROR");
          return;
        }

        let zipReader = Cc["@mozilla.org/libjar/zip-reader;1"]
                        .createInstance(Ci.nsIZipReader);
        try {
          zipReader.open(zipFile);
          if (!zipReader.hasEntry("manifest.webapp")) {
            throw "No manifest.webapp found.";
          }

          let istream = zipReader.getInputStream("manifest.webapp");

          
          let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                          .createInstance(Ci.nsIScriptableUnicodeConverter);
          converter.charset = "UTF-8";

          let manifest = JSON.parse(converter.ConvertToUnicode(NetUtil.readInputStreamToString(istream,
                                                               istream.available()) || ""));

          if (!AppsUtils.checkManifest(manifest, aApp.installOrigin)) {
            throw "INVALID_MANIFEST";
          }

          if (!checkAppStatus(manifest)) {
            throw "INVALID_SECURITY_LEVEL";
          }

          if (aOnSuccess) {
            aOnSuccess(id, manifest);
          }
          delete DOMApplicationRegistry.downloads[aApp.manifestURL];
        } catch (e) {
          
          cleanup(e);
        } finally {
          zipReader.close();
        }
      });
    });
  },

  uninstall: function(aData, aMm) {
    for (let id in this.webapps) {
      let app = this.webapps[id];
      if (app.origin != aData.origin) {
        continue;
      }

      if (!this.webapps[id].removable)
        return;

      
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
        this.broadcastMessage("Webapps:Uninstall:Return:OK", aData);
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
    if (!id || this.webapps[id].installState == "pending") {
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
        delete this.webapps[record.id];
        let dir = this._getAppDir(record.id);
        try {
          dir.remove(true);
        } catch (e) {
        }
        this.broadcastMessage("Webapps:Uninstall:Return:OK", { origin: origin });
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
    let xdg_data_home_env = env.get("XDG_DATA_HOME");

    let desktopINI;
    if (xdg_data_home_env != "") {
      desktopINI = Cc["@mozilla.org/file/local;1"]
                     .createInstance(Ci.nsIFile);
      desktopINI.initWithPath(xdg_data_home_env);
    }
    else {
      desktopINI = Services.dirsvc.get("Home", Ci.nsIFile);
      desktopINI.append(".local");
      desktopINI.append("share");
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
  this.app = aApp;
  this.startStatus = aApp.installState;
};

AppcacheObserver.prototype = {
  
  updateStateChanged: function appObs_Update(aUpdate, aState) {
    let mustSave = false;
    let app = this.app;

    let setStatus = function appObs_setStatus(aStatus) {
      mustSave = (app.installState != aStatus);
      app.installState = aStatus;
      app.downloading = false;
      DOMApplicationRegistry.broadcastMessage("Webapps:OfflineCache",
                                              { manifest: app.manifestURL,
                                                installState: app.installState });
    }

    let setError = function appObs_setError(aError) {
      DOMApplicationRegistry.broadcastMessage("Webapps:OfflineCache",
                                              { manifest: app.manifestURL,
                                                error: aError });
      app.downloading = false;
      mustSave = true;
    }

    switch (aState) {
      case Ci.nsIOfflineCacheUpdateObserver.STATE_ERROR:
        aUpdate.removeObserver(this);
        setError("APP_CACHE_DOWNLOAD_ERROR");
        break;
      case Ci.nsIOfflineCacheUpdateObserver.STATE_NOUPDATE:
      case Ci.nsIOfflineCacheUpdateObserver.STATE_FINISHED:
        aUpdate.removeObserver(this);
        setStatus("installed");
        break;
      case Ci.nsIOfflineCacheUpdateObserver.STATE_DOWNLOADING:
      case Ci.nsIOfflineCacheUpdateObserver.STATE_ITEMSTARTED:
      case Ci.nsIOfflineCacheUpdateObserver.STATE_ITEMPROGRESS:
        setStatus(this.startStatus);
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
