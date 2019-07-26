



"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;


const SEC_ERROR_BASE = Ci.nsINSSErrorsService.NSS_SEC_ERROR_BASE;
const SEC_ERROR_EXPIRED_CERTIFICATE = (SEC_ERROR_BASE + 11);



function buildIDToTime() {
  let platformBuildID =
    Cc["@mozilla.org/xre/app-info;1"]
      .getService(Ci.nsIXULAppInfo).platformBuildID;
  let platformBuildIDDate = new Date();
  platformBuildIDDate.setUTCFullYear(platformBuildID.substr(0,4),
                                      platformBuildID.substr(4,2) - 1,
                                      platformBuildID.substr(6,2));
  platformBuildIDDate.setUTCHours(platformBuildID.substr(8,2),
                                  platformBuildID.substr(10,2),
                                  platformBuildID.substr(12,2));
  return platformBuildIDDate.getTime();
}

const PLATFORM_BUILD_ID_TIME = buildIDToTime();

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
Cu.import("resource://gre/modules/WebappOSUtils.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

#ifdef MOZ_WIDGET_GONK
XPCOMUtils.defineLazyGetter(this, "libcutils", function() {
  Cu.import("resource://gre/modules/systemlibs.js");
  return libcutils;
});
#endif

function debug(aMsg) {
#ifdef MOZ_DEBUG
  dump("-*- Webapps.jsm : " + aMsg + "\n");
#endif
}

function getNSPRErrorCode(err) {
  return -1 * ((err) & 0xffff);
}

function supportUseCurrentProfile() {
  return Services.prefs.getBoolPref("dom.webapps.useCurrentProfile");
}

function supportSystemMessages() {
  return Services.prefs.getBoolPref("dom.sysmsg.enabled");
}


const MIN_PROGRESS_EVENT_DELAY = 1500;

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

XPCOMUtils.defineLazyGetter(this, "interAppCommService", function() {
  return Cc["@mozilla.org/inter-app-communication-service;1"]
         .getService(Ci.nsIInterAppCommService);
});

XPCOMUtils.defineLazyServiceGetter(this, "dataStoreService",
                                   "@mozilla.org/datastore-service;1",
                                   "nsIDataStoreService");

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






const STORE_ID_PENDING_PREFIX = "#unknownID#";

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
            if (!app) {
              delete this.webapps[id];
              continue;
            }

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

            
            if (this.webapps[id].storeId === undefined) {
              this.webapps[id].storeId = "";
            }
            if (this.webapps[id].storeVersion === undefined) {
              this.webapps[id].storeVersion = 0;
            }

            
            if (this.webapps[id].role === undefined) {
              this.webapps[id].role = "";
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

  
  sanitizeRedirects: function sanitizeRedirects(aSource) {
    if (!aSource) {
      return null;
    }

    let res = [];
    for (let i = 0; i < aSource.length; i++) {
      let redirect = aSource[i];
      if (redirect.from && redirect.to &&
          isAbsoluteURI(redirect.from) &&
          !isAbsoluteURI(redirect.to)) {
        res.push(redirect);
      }
    }
    return res.length > 0 ? res : null;
  },

  
  registerAppsHandlers: function registerAppsHandlers(aRunUpdate) {
    this.notifyAppsRegistryStart();
    let ids = [];
    for (let id in this.webapps) {
      ids.push({ id: id });
    }
    if (supportSystemMessages()) {
      this._processManifestForIds(ids, aRunUpdate);
    } else {
      
      
      
      this._readManifests(ids, (function readCSPs(aResults) {
        aResults.forEach(function registerManifest(aResult) {
          if (!aResult.manifest) {
            
            
            delete this.webapps[aResult.id];
            return;
          }
          let app = this.webapps[aResult.id];
          app.csp = aResult.manifest.csp || "";
          app.role = aResult.manifest.role || "";
          if (app.appStatus >= Ci.nsIPrincipal.APP_STATUS_PRIVILEGED) {
            app.redirects = this.sanitizeRedirects(aResult.redirects);
          }
        }, this);
      }).bind(this));

      
      this.notifyAppsRegistryReady();
    }
  },

  updateDataStoreForApp: function(aId) {
    if (!this.webapps[aId]) {
      return;
    }

    
    this._readManifests([{ id: aId }], (function(aResult) {
      let app = this.webapps[aId];
      this.updateDataStore(app.localId, app.origin, app.manifestURL,
                           aResult[0].manifest, app.appStatus);
    }).bind(this));
  },

  updatePermissionsForApp: function updatePermissionsForApp(aId) {
    if (!this.webapps[aId]) {
      return;
    }

    
    
    
    if (supportUseCurrentProfile()) {
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
    }
  },

  updateOfflineCacheForApp: function updateOfflineCacheForApp(aId) {
    let app = this.webapps[aId];
    this._readManifests([{ id: aId }], function(aResult) {
      let manifest = new ManifestHelper(aResult[0].manifest, app.origin);
      OfflineCacheInstaller.installCache({
        cachePath: app.cachePath,
        appId: aId,
        origin: Services.io.newURI(app.origin, null, null),
        localId: app.localId,
        appcache_path: manifest.fullAppcachePath()
      });
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
      } else if (!baseDir.directoryEntries.hasMoreElements()) {
        debug("Error: Core app in " + baseDir.path + " is empty");
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
        try {
          file.copyTo(destDir, aFile);
        } catch(e) {
          debug("Error: Failed to copy " + file.path + " to " + destDir.path);
        }
      });

    app.installState = "installed";
    app.cachePath = app.basePath;
    app.basePath = FileUtils.getDir(DIRECTORY_NAME, ["webapps"], true, true)
                            .path;

    if (!isPackage) {
      return;
    }

    app.origin = "app://" + aId;

    
    
    
    app.storeId = STORE_ID_PENDING_PREFIX + app.installOrigin;

    
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

  
  
  removeIfHttpsDuplicate: function(aId) {
#ifdef MOZ_WIDGET_GONK
    let app = this.webapps[aId];
    if (!app || !app.origin.startsWith("http://")) {
      return;
    }

    let httpsManifestURL =
      "https://" + app.manifestURL.substring("http://".length);

    
    
    for (let id in this.webapps) {
       if (this.webapps[id].manifestURL === httpsManifestURL) {
         debug("Found a http/https match: " + app.manifestURL + " / " +
               this.webapps[id].manifestURL);
         this.uninstall(app.manifestURL, function() {}, function() {});
         return;
       }
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
          
          let localId = this.webapps[id].localId;
          let permMgr = Cc["@mozilla.org/permissionmanager;1"]
                          .getService(Ci.nsIPermissionManager);
          permMgr.removePermissionsForApp(localId, false);
          Services.cookies.removeCookiesForApp(localId, false);
          this._clearPrivateData(localId, false);
          delete this.webapps[id];
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

    if (!idbDir.exists() || !idbDir.isDirectory()) {
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
          this.removeIfHttpsDuplicate(id);
          if (!this.webapps[id]) {
            continue;
          }
          this.updateOfflineCacheForApp(id);
          this.updatePermissionsForApp(id);
        }
        
        
        this._saveApps();
      }

      
      for (let id in this.webapps) {
        this.updateDataStoreForApp(id);
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

  updateDataStore: function(aId, aOrigin, aManifestURL, aManifest, aAppStatus) {
    
    let prefName = "dom.testing.datastore_enabled_for_hosted_apps";
    if (aAppStatus != Ci.nsIPrincipal.APP_STATUS_CERTIFIED &&
        (Services.prefs.getPrefType(prefName) == Services.prefs.PREF_INVALID ||
         !Services.prefs.getBoolPref(prefName))) {
      return;
    }

    if ('datastores-owned' in aManifest) {
      for (let name in aManifest['datastores-owned']) {
        let readonly = "access" in aManifest['datastores-owned'][name]
                         ? aManifest['datastores-owned'][name].access == 'readonly'
                         : false;

        dataStoreService.installDataStore(aId, name, aOrigin, aManifestURL,
                                          readonly);
      }
    }

    if ('datastores-access' in aManifest) {
      for (let name in aManifest['datastores-access']) {
        let readonly = ("readonly" in aManifest['datastores-access'][name]) &&
                       !aManifest['datastores-access'][name].readonly
                         ? false : true;

        dataStoreService.installAccessDataStore(aId, name, aOrigin,
                                                aManifestURL, readonly);
      }
    }
  },

  
  
  
  
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

  
  
  
  
  _registerInterAppConnectionsForEntryPoint: function(aManifest, aApp,
                                                      aEntryPoint) {
    let root = aManifest;
    if (aEntryPoint && aManifest.entry_points[aEntryPoint]) {
      root = aManifest.entry_points[aEntryPoint];
    }

    let connections = root.connections;
    if (!connections) {
      return;
    }

    if ((typeof connections) !== "object") {
      debug("|connections| is not an object. Skipping: " + connections);
      return;
    }

    let manifest = new ManifestHelper(aManifest, aApp.origin);
    let launchPathURI = Services.io.newURI(manifest.fullLaunchPath(aEntryPoint),
                                           null, null);
    let manifestURI = Services.io.newURI(aApp.manifestURL, null, null);

    for (let keyword in connections) {
      let connection = connections[keyword];

      
      
      let fullHandlerPath;
      let handlerPath = connection.handler_path;
      if (handlerPath) {
        try {
          fullHandlerPath = manifest.resolveFromOrigin(handlerPath);
        } catch(e) {
          debug("Connection's handler path is invalid. Skipping: keyword: " +
                keyword + " handler_path: " + handlerPath);
          continue;
        }
      }
      let handlerPageURI = fullHandlerPath
                           ? Services.io.newURI(fullHandlerPath, null, null)
                           : launchPathURI;

      if (SystemMessagePermissionsChecker
            .isSystemMessagePermittedToRegister("connection",
                                                aApp.origin,
                                                aManifest)) {
        msgmgr.registerPage("connection", handlerPageURI, manifestURI);
      }

      interAppCommService.
        registerConnection(keyword,
                           handlerPageURI,
                           manifestURI,
                           connection.description,
                           AppsUtils.getAppManifestStatus(manifest),
                           connection.rules);
    }
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

  _registerInterAppConnections: function(aManifest, aApp) {
    this._registerInterAppConnectionsForEntryPoint(aManifest, aApp, null);

    if (!aManifest.entry_points) {
      return;
    }

    for (let entryPoint in aManifest.entry_points) {
      this._registerInterAppConnectionsForEntryPoint(aManifest, aApp,
                                                     entryPoint);
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
                                    "name": activity,
                                    "description": description });
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
        if (!manifest) {
          
          
          delete this.webapps[aResult.id];
          return;
        }
        app.name = manifest.name;
        app.csp = manifest.csp || "";
        app.role = manifest.role || "";
        if (app.appStatus >= Ci.nsIPrincipal.APP_STATUS_PRIVILEGED) {
          app.redirects = this.sanitizeRedirects(manifest.redirects);
        }
        this._registerSystemMessages(manifest, app);
        this._registerInterAppConnections(manifest, app);
        appsToRegister.push({ manifest: manifest, app: app });
      }, this);
      this._registerActivitiesForApps(appsToRegister, aRunUpdate);
    }).bind(this));
  },

  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "xpcom-shutdown") {
      this.messages.forEach((function(msgName) {
        ppmm.removeMessageListener(msgName, this);
      }).bind(this));
      Services.obs.removeObserver(this, "xpcom-shutdown");
      cpmm = null;
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

  addMessageListener: function(aMsgNames, aApp, aMm) {
    aMsgNames.forEach(function (aMsgName) {
      let man = aApp && aApp.manifestURL;
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

      
      if ((aMsgName === 'Webapps:FireEvent') ||
          (aMsgName === 'Webapps:UpdateState')) {
        if (man) {
          let app = this.getAppByManifestURL(aApp.manifestURL);
          if (app && ((aApp.installState !== app.installState) ||
                      (aApp.downloading !== app.downloading))) {
            debug("Got a registration from an outdated app: " +
                  aApp.manifestURL);
            let aEvent ={
              type: app.installState,
              app: app,
              manifestURL: app.manifestURL,
              manifest: app.manifest
            };
            aMm.sendAsyncMessage(aMsgName, aEvent);
          }
        }
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
        this.doUninstall(msg, mm);
        break;
      case "Webapps:Launch":
        this.doLaunch(msg, mm);
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
        this.doGetAll(msg, mm);
        break;
      case "Webapps:InstallPackage":
        this.doInstallPackage(msg, mm);
        break;
      case "Webapps:RegisterForMessages":
        this.addMessageListener(msg.messages, msg.app, mm);
        break;
      case "Webapps:UnregisterForMessages":
        this.removeMessageListener(msg, mm);
        break;
      case "child-process-shutdown":
        this.removeMessageListener(["Webapps:Internal:AllMessages"], mm);
        break;
      case "Webapps:GetList":
        this.addMessageListener(["Webapps:AddApp", "Webapps:RemoveApp"], null, mm);
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
    return AppsUtils.getAppInfo(this.webapps, aAppId);
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

  _writeFile: function _writeFile(aFile, aData, aCallback) {
    debug("Saving " + aFile.path);
    
    let ostream = FileUtils.openSafeFileOutputStream(aFile);

    
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    
    let istream = converter.convertToInputStream(aData);
    NetUtil.asyncCopy(istream, ostream, function(rc) {
      if (aCallback) {
        aCallback();
      }
    });
  },

  doLaunch: function (aData, aMm) {
    this.launch(
      aData.manifestURL,
      aData.startPoint,
      aData.timestamp,
      function onsuccess() {
        aMm.sendAsyncMessage("Webapps:Launch:Return:OK", aData);
      },
      function onfailure(reason) {
        aMm.sendAsyncMessage("Webapps:Launch:Return:KO", aData);
      }
    );
  },

  launch: function launch(aManifestURL, aStartPoint, aTimeStamp, aOnSuccess, aOnFailure) {
    let app = this.getAppByManifestURL(aManifestURL);
    if (!app) {
      aOnFailure("NO_SUCH_APP");
      return;
    }

    
    
    if (app.installState == "pending") {
      aOnFailure("PENDING_APP_NOT_LAUNCHABLE");
      return;
    }

    
    
    let appClone = AppsUtils.cloneAppObject(app);
    appClone.startPoint = aStartPoint;
    appClone.timestamp = aTimeStamp;
    Services.obs.notifyObservers(null, "webapps-launch", JSON.stringify(appClone));
    aOnSuccess();
  },

  close: function close(aApp) {
    debug("close");

    
    
    let appClone = AppsUtils.cloneAppObject(aApp);
    Services.obs.notifyObservers(null, "webapps-close", JSON.stringify(appClone));
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
      try {
        download.cacheUpdate.cancel();
      } catch (e) {
        debug (e);
      }
    } else if (download.channel) {
      try {
        download.channel.cancel(Cr.NS_BINDING_ABORTED);
      } catch(e) { }
    } else {
      return;
    }

    this._saveApps((function() {
      this.broadcastMessage("Webapps:UpdateState", {
        app: {
          progress: 0,
          installState: download.previousState,
          downloading: false
        },
        error: error,
        manifestURL: app.manifestURL,
      })
      this.broadcastMessage("Webapps:FireEvent", {
        eventType: "downloaderror",
        manifestURL: app.manifestURL
      });
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
      this.broadcastMessage("Webapps:UpdateState", {
        error: "NO_DOWNLOAD_AVAILABLE",
        manifestURL: app.manifestURL
      });
      this.broadcastMessage("Webapps:FireEvent", {
        eventType: "downloaderror",
        manifestURL: app.manifestURL
      });
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
          this.startOfflineCacheDownload(manifest, app, null, isUpdate);
        } else {
          
          
          debug("No appcache found, sending 'downloaded' for " + aManifestURL);
          app.downloadAvailable = false;
          DOMApplicationRegistry._saveApps(function() {
            DOMApplicationRegistry.broadcastMessage("Webapps:UpdateState", {
              app: app,
              manifest: jsonManifest,
              manifestURL: aManifestURL
            });
            DOMApplicationRegistry.broadcastMessage("Webapps:FireEvent", {
              eventType: "downloadsuccess",
              manifestURL: aManifestURL
            });
          });
        }
      }).bind(this));

      return;
    }

    this._loadJSONAsync(file, (function(aJSON) {
      if (!aJSON) {
        debug("startDownload: No update manifest found at " + file.path + " " +
              aManifestURL);
        return;
      }

      let manifest = new ManifestHelper(aJSON, app.installOrigin);
      this.downloadPackage(manifest, {
          manifestURL: aManifestURL,
          origin: app.origin,
          installOrigin: app.installOrigin,
          downloadSize: app.downloadSize
        }, isUpdate).then(function([aId, aManifest]) {
          
          
          let tmpDir = FileUtils.getDir("TmpD", ["webapps", aId], true, true);

          
          let manFile = tmpDir.clone();
          manFile.append("manifest.webapp");
          DOMApplicationRegistry._writeFile(manFile,
                                            JSON.stringify(aManifest),
                                            function() { });

          app = DOMApplicationRegistry.webapps[aId];
          
          app.downloading = false;
          app.downloadAvailable = false;
          app.readyToApplyDownload = true;
          app.updateTime = Date.now();
          DOMApplicationRegistry._saveApps(function() {
            DOMApplicationRegistry.broadcastMessage("Webapps:UpdateState", {
              app: app,
              manifestURL: aManifestURL
            });
            DOMApplicationRegistry.broadcastMessage("Webapps:FireEvent", {
              eventType: "downloadsuccess",
              manifestURL: aManifestURL
            });
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

    
    this.getManifestFor(aManifestURL, (function(aOldManifest) {
      
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

      
      if (id in this._manifestCache) {
        delete this._manifestCache[id];
      }

      
      
      let zipFile = dir.clone();
      zipFile.append("application.zip");
      Services.obs.notifyObservers(zipFile, "flush-cache-entry", null);

      
      this.getManifestFor(aManifestURL, (function(aData) {
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

        this._saveApps((function() {
          
          this.updateAppHandlers(aOldManifest, aData, app);
          if (supportUseCurrentProfile()) {
            PermissionsInstaller.installPermissions(
              { manifest: aData,
                origin: app.origin,
                manifestURL: app.manifestURL },
              true);
          }
          this.updateDataStore(this.webapps[id].localId, app.origin,
                               app.manifestURL, aData, app.appStatus);
          this.broadcastMessage("Webapps:UpdateState", {
            app: app,
            manifest: aData,
            manifestURL: app.manifestURL
          });
          this.broadcastMessage("Webapps:FireEvent", {
            eventType: "downloadapplied",
            manifestURL: app.manifestURL
          });
        }).bind(this));
      }).bind(this));
    }).bind(this));
  },

  startOfflineCacheDownload: function(aManifest, aApp, aProfileDir, aIsUpdate) {
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
    aApp.progress = 0;
    DOMApplicationRegistry._saveApps((function() {
      DOMApplicationRegistry.broadcastMessage("Webapps:UpdateState", {
        app: {
          downloading: true,
          installState: aApp.installState,
          progress: 0
        },
        manifestURL: aApp.manifestURL
      });
      let cacheUpdate = aProfileDir
        ? updateSvc.scheduleCustomProfileUpdate(appcacheURI, docURI, aProfileDir)
        : updateSvc.scheduleAppUpdate(appcacheURI, docURI, aApp.localId, false);

      
      
      let download = {
        cacheUpdate: cacheUpdate,
        appId: this._appIdForManifestURL(aApp.manifestURL),
        previousState: aIsUpdate ? "installed" : "pending"
      };
      AppDownloadManager.add(aApp.manifestURL, download);

      cacheUpdate.addObserver(new AppcacheObserver(aApp), false);

    }).bind(this));
  },

  
  computeManifestHash: function(aManifest) {
    return AppsUtils.computeHash(JSON.stringify(aManifest));
  },

  
  
  updateAppHandlers: function(aOldManifest, aNewManifest, aApp) {
    debug("updateAppHandlers: old=" + aOldManifest + " new=" + aNewManifest);
    this.notifyAppsRegistryStart();
    if (aApp.appStatus >= Ci.nsIPrincipal.APP_STATUS_PRIVILEGED) {
      aApp.redirects = this.sanitizeRedirects(aNewManifest.redirects);
    }

    if (supportSystemMessages()) {
      if (aOldManifest) {
        this._unregisterActivities(aOldManifest, aApp);
      }
      this._registerSystemMessages(aNewManifest, aApp);
      this._registerActivities(aNewManifest, aApp, true);
      this._registerInterAppConnections(aNewManifest, aApp);
    } else {
      
      this.notifyAppsRegistryReady();
    }
  },

  checkForUpdate: function(aData, aMm) {
    debug("checkForUpdate for " + aData.manifestURL);

    function sendError(aError) {
      aData.error = aError;
      aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:KO", aData);
    }

    let id = this._appIdForManifestURL(aData.manifestURL);
    let app = this.webapps[id];

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
      app.updateManifest = aManifest;
      DOMApplicationRegistry._saveApps(function() {
        DOMApplicationRegistry.broadcastMessage("Webapps:UpdateState", {
          app: app,
          manifestURL: app.manifestURL
        });
        DOMApplicationRegistry.broadcastMessage("Webapps:FireEvent", {
          eventType: "downloadavailable",
          manifestURL: app.manifestURL,
          requestID: aData.requestID
        });
      });
    }

    
    
    
    
    
    
    function updateHostedApp(aOldManifest, aNewManifest) {
      debug("updateHostedApp " + aData.manifestURL);

      
      if (id in this._manifestCache) {
        delete this._manifestCache[id];
      }

      app.manifest = aNewManifest || aOldManifest;

      let manifest;
      if (aNewManifest) {
        this.updateAppHandlers(aOldManifest, aNewManifest, app);

        
        let dir = FileUtils.getDir(DIRECTORY_NAME, ["webapps", id], true, true);
        let manFile = dir.clone();
        manFile.append("manifest.webapp");
        this._writeFile(manFile, JSON.stringify(aNewManifest), function() { });
        manifest = new ManifestHelper(aNewManifest, app.origin);

        if (supportUseCurrentProfile()) {
          
          PermissionsInstaller.installPermissions({
            manifest: app.manifest,
            origin: app.origin,
            manifestURL: aData.manifestURL
          }, true);
        }

        this.updateDataStore(this.webapps[id].localId, app.origin,
                             app.manifestURL, app.manifest, app.appStatus);

        app.name = manifest.name;
        app.csp = manifest.csp || "";
        app.role = manifest.role || "";
        app.updateTime = Date.now();
      } else {
        manifest = new ManifestHelper(aOldManifest, app.origin);
      }

      
      this.webapps[id] = app;
      this._saveApps(function() {
        let reg = DOMApplicationRegistry;
        if (!manifest.appcache_path) {
          reg.broadcastMessage("Webapps:UpdateState", {
            app: app,
            manifest: app.manifest,
            manifestURL: app.manifestURL
          });
          reg.broadcastMessage("Webapps:FireEvent", {
            eventType: "downloadapplied",
            manifestURL: app.manifestURL,
            requestID: aData.requestID
          });
        } else {
          
          
          let updateObserver = {
            observe: function(aSubject, aTopic, aObsData) {
              debug("updateHostedApp: updateSvc.checkForUpdate return for " +
                    app.manifestURL + " - event is " + aTopic);
              let eventType =
                aTopic == "offline-cache-update-available" ? "downloadavailable"
                                                           : "downloadapplied";
              app.downloadAvailable = (eventType == "downloadavailable");
              reg._saveApps(function() {
                reg.broadcastMessage("Webapps:UpdateState", {
                  app: app,
                  manifest: app.manifest,
                  manifestURL: app.manifestURL
                });
                reg.broadcastMessage("Webapps:FireEvent", {
                  eventType: eventType,
                  manifestURL: app.manifestURL,
                  requestID: aData.requestID
                });
              });
            }
          };
          debug("updateHostedApp: updateSvc.checkForUpdate for " +
                manifest.fullAppcachePath());
          updateSvc.checkForUpdate(Services.io.newURI(manifest.fullAppcachePath(), null, null),
                                   app.localId, false, updateObserver);
        }
        delete app.manifest;
      });
    }


    
    if (!app) {
      sendError("NO_SUCH_APP");
      return;
    }

    
    if (app.installState !== "installed") {
      sendError("PENDING_APP_NOT_UPDATABLE");
      return;
    }

    
    if (app.downloading) {
      sendError("APP_IS_DOWNLOADING");
      return;
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

        debug("Checking only appcache for " + aData.manifestURL);
        
        
        let updateObserver = {
          observe: function(aSubject, aTopic, aObsData) {
            debug("onlyCheckAppCache updateSvc.checkForUpdate return for " +
                  app.manifestURL + " - event is " + aTopic);
            if (aTopic == "offline-cache-update-available") {
              app.downloadAvailable = true;
              this._saveApps(function() {
                DOMApplicationRegistry.broadcastMessage("Webapps:UpdateState", {
                  app: app,
                  manifestURL: app.manifestURL
                });
                DOMApplicationRegistry.broadcastMessage("Webapps:FireEvent", {
                  eventType: "downloadavailable",
                  manifestURL: app.manifestURL,
                  requestID: aData.requestID
                });
              });
            } else {
              aData.error = "NOT_UPDATABLE";
              aMm.sendAsyncMessage("Webapps:CheckForUpdate:Return:KO", aData);
            }
          }
        };
        let helper = new ManifestHelper(manifest);
        debug("onlyCheckAppCache - launch updateSvc.checkForUpdate for " +
              helper.fullAppcachePath());
        updateSvc.checkForUpdate(Services.io.newURI(helper.fullAppcachePath(), null, null),
                                 app.localId, false, updateObserver);
      });
      return;
    }

    
    function onload(xhr, oldManifest) {
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
          AppsUtils.ensureSameAppName(oldManifest, manifest, app);

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
              this._saveApps(function() {
                
                
                let eventType = app.downloadAvailable ? "downloadavailable"
                                                      : "downloadapplied";
                aMm.sendAsyncMessage("Webapps:UpdateState", {
                  app: app,
                  manifestURL: app.manifestURL
                });
                aMm.sendAsyncMessage("Webapps:FireEvent", {
                  eventType: eventType,
                  manifestURL: app.manifestURL,
                  requestID: aData.requestID
                });
              });
            }
          } else {
            
            
            updateHostedApp.call(this, oldManifest,
                                 oldHash == hash ? null : manifest);
          }
        }
      } else if (xhr.status == 304) {
        
        if (isPackage) {
          app.lastCheckedUpdate = Date.now();
          this._saveApps(function() {
            
            
            let eventType = app.downloadAvailable ? "downloadavailable"
                                                  : "downloadapplied";
            aMm.sendAsyncMessage("Webapps:UpdateState", {
              app: app,
              manifestURL: app.manifestURL
            });
            aMm.sendAsyncMessage("Webapps:FireEvent", {
              eventType: eventType,
              manifestURL: app.manifestURL,
              requestID: aData.requestID
            });
          });
        } else {
          
          
          updateHostedApp.call(this, oldManifest, null);
        }
      } else {
        sendError("MANIFEST_URL_ERROR");
      }
    }

    
    function doRequest(oldManifest, headers) {
      headers = headers || [];
      let xhr = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"]
                  .createInstance(Ci.nsIXMLHttpRequest);
      xhr.open("GET", aData.manifestURL, true);
      xhr.channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
      headers.forEach(function(aHeader) {
        debug("Adding header: " + aHeader.name + ": " + aHeader.value);
        xhr.setRequestHeader(aHeader.name, aHeader.value);
      });
      xhr.responseType = "json";
      if (app.etag) {
        debug("adding manifest etag:" + app.etag);
        xhr.setRequestHeader("If-None-Match", app.etag);
      }
      xhr.channel.notificationCallbacks =
        this.createLoadContext(app.installerAppId, app.installerIsBrowser);

      xhr.addEventListener("load", onload.bind(this, xhr, oldManifest), false);
      xhr.addEventListener("error", (function() {
        sendError("NETWORK_ERROR");
      }).bind(this), false);

      debug("Checking manifest at " + aData.manifestURL);
      xhr.send(null);
    }

    
    this._readManifests([{ id: id }], (function(aResult) {
      let extraHeaders = [];
#ifdef MOZ_WIDGET_GONK
      let pingManifestURL;
      try {
        pingManifestURL = Services.prefs.getCharPref("ping.manifestURL");
      } catch(e) { }

      if (pingManifestURL && pingManifestURL == aData.manifestURL) {
        
        let device = libcutils.property_get("ro.product.model");
        extraHeaders.push({ name: "X-MOZ-B2G-DEVICE",
                            value: device || "unknown" });
      }
#endif
      doRequest.call(this, aResult[0].manifest, extraHeaders);
    }).bind(this));
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

        
        
        
        for (let id in this.webapps) {
          if (this.webapps[id].origin == app.origin &&
              !this.webapps[id].packageHash &&
              this._isLaunchable(this.webapps[id])) {
            sendError("MULTIPLE_APPS_PER_ORIGIN_FORBIDDEN");
            return;
          }
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

        
        let id = this._appIdForManifestURL(app.manifestURL);
        if (id !== null && this._isLaunchable(this.webapps[id])) {
          sendError("REINSTALL_FORBIDDEN");
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
          
          
          let prefName = "dom.mozApps.auto_confirm_install";
          if (Services.prefs.prefHasUserValue(prefName) &&
              Services.prefs.getBoolPref(prefName)) {
            this.confirmInstall(aData);
          } else {
            Services.obs.notifyObservers(aMm, "webapps-ask-install",
                                         JSON.stringify(aData));
          }
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
  queuedPackageDownload: {},

onInstallSuccessAck: function onInstallSuccessAck(aManifestURL,
                                                  aDontNeedNetwork) {
    
    if ((Services.io.offline) && !aDontNeedNetwork) {
      let onlineWrapper = {
        observe: function(aSubject, aTopic, aData) {
          Services.obs.removeObserver(onlineWrapper,
                                      "network:offline-status-changed");
          DOMApplicationRegistry.onInstallSuccessAck(aManifestURL);
        }
      };
      Services.obs.addObserver(onlineWrapper,
                               "network:offline-status-changed", false);
      return;
    }

    let cacheDownload = this.queuedDownload[aManifestURL];
    if (cacheDownload) {
      this.startOfflineCacheDownload(cacheDownload.manifest,
                                     cacheDownload.app,
                                     cacheDownload.profileDir);
      delete this.queuedDownload[aManifestURL];

      return;
    }

    let packageDownload = this.queuedPackageDownload[aManifestURL];
    if (packageDownload) {
      let manifest = packageDownload.manifest;
      let newApp = packageDownload.app;
      let installSuccessCallback = packageDownload.callback;

      delete this.queuedPackageDownload[aManifestURL];

      this.downloadPackage(manifest, newApp, false).then(
        this._onDownloadPackage.bind(this, newApp, installSuccessCallback)
      );
    }
  },

  _setupApp: function(aData, aId) {
    let app = aData.app;

    
    app.removable = true;

    if (aData.isPackage) {
      
      app.origin = "app://" + aId;
    }

    app.id = aId;
    app.installTime = Date.now();
    app.lastUpdateCheck = Date.now();

    return app;
  },

  _cloneApp: function(aData, aNewApp, aManifest, aId, aLocalId) {
    let appObject = AppsUtils.cloneAppObject(aNewApp);
    appObject.appStatus =
      aNewApp.appStatus || Ci.nsIPrincipal.APP_STATUS_INSTALLED;

    if (aManifest.appcache_path) {
      appObject.installState = "pending";
      appObject.downloadAvailable = true;
      appObject.downloading = true;
      appObject.downloadSize = 0;
      appObject.readyToApplyDownload = false;
    } else if (aManifest.package_path) {
      appObject.installState = "pending";
      appObject.downloadAvailable = true;
      appObject.downloading = true;
      appObject.downloadSize = aManifest.size;
      appObject.readyToApplyDownload = false;
    } else {
      appObject.installState = "installed";
      appObject.downloadAvailable = false;
      appObject.downloading = false;
      appObject.readyToApplyDownload = false;
    }

    appObject.localId = aLocalId;
    appObject.basePath =
      FileUtils.getDir(DIRECTORY_NAME, ["webapps"], true, true).path;
    appObject.name = aManifest.name;
    appObject.csp = aManifest.csp || "";
    appObject.role = aManifest.role || "";
    appObject.installerAppId = aData.appId;
    appObject.installerIsBrowser = aData.isBrowser;

    return appObject;
  },

  _writeManifestFile: function(aId, aIsPackage, aJsonManifest) {
    debug("_writeManifestFile");
    let dir = this._getAppDir(aId);
    let manFile = dir.clone();

    
    let manifestName = aIsPackage ? "update.webapp" : "manifest.webapp";
    manFile.append(manifestName);
    this._writeFile(manFile, JSON.stringify(aJsonManifest), function() { });
  },

  confirmInstall: function(aData, aProfileDir, aInstallSuccessCallback) {
    debug("confirmInstall");

    let origin = Services.io.newURI(aData.app.origin, null, null);
    let id = this._appIdForManifestURL(aData.app.manifestURL);
    let manifestURL = origin.resolve(aData.app.manifestURL);
    let localId = this.getAppLocalIdByManifestURL(manifestURL);

    let isReinstall = false;

    
    if (id) {
      isReinstall = true;
      let dir = this._getAppDir(id);
      try {
        dir.remove(true);
      } catch(e) { }
    } else {
      id = this.makeAppId();
      localId = this._nextLocalId();
    }

    let app = this._setupApp(aData, id);

    let jsonManifest = aData.isPackage ? app.updateManifest : app.manifest;
    this._writeManifestFile(id, aData.isPackage, jsonManifest);

    debug("app.origin: " + app.origin);
    let manifest = new ManifestHelper(jsonManifest, app.origin);

    let appObject = this._cloneApp(aData, app, manifest, id, localId);

    this.webapps[id] = appObject;

    
    
    if (!aData.isPackage) {
      if (supportUseCurrentProfile()) {
        PermissionsInstaller.installPermissions(
          {
            origin: appObject.origin,
            manifestURL: appObject.manifestURL,
            manifest: jsonManifest
          },
          isReinstall,
          this.uninstall.bind(this, aData, aData.mm)
        );
      }

      this.updateDataStore(this.webapps[id].localId,  this.webapps[id].origin,
                           this.webapps[id].manifestURL, jsonManifest,
                           this.webapps[id].appStatus);
    }

    for each (let prop in ["installState", "downloadAvailable", "downloading",
                           "downloadSize", "readyToApplyDownload"]) {
      aData.app[prop] = appObject[prop];
    }

    if (manifest.appcache_path) {
      this.queuedDownload[app.manifestURL] = {
        manifest: manifest,
        app: appObject,
        profileDir: aProfileDir
      }
    }

    
    
    
    this._saveApps((function() {
      this.broadcastMessage("Webapps:AddApp", { id: id, app: appObject });
      this.broadcastMessage("Webapps:Install:Return:OK", aData);
      Services.obs.notifyObservers(null, "webapps-installed",
        JSON.stringify({ manifestURL: app.manifestURL }));
    }).bind(this));

    if (!aData.isPackage) {
      this.updateAppHandlers(null, app.manifest, app);
      if (aInstallSuccessCallback) {
        aInstallSuccessCallback(app.manifest);
      }
    }
    let dontNeedNetwork = false;
    if (manifest.package_path) {
      
      
      let origPath = jsonManifest.package_path;
      if (aData.app.localInstallPath) {
        dontNeedNetwork = true;
        jsonManifest.package_path = "file://" + aData.app.localInstallPath;
      }
      
      
      manifest = new ManifestHelper(jsonManifest, app.manifestURL);

      this.queuedPackageDownload[app.manifestURL] = {
        manifest: manifest,
        app: appObject,
        callback: aInstallSuccessCallback
      };
    }

    if (aData.forceSuccessAck) {
      
      
      this.onInstallSuccessAck(app.manifestURL, dontNeedNetwork);
    }
  },















  _onDownloadPackage: function(aNewApp, aInstallSuccessCallback,
                               [aId, aManifest]) {
    debug("_onDownloadPackage");
    
    let app = this.webapps[aId];
    let zipFile =
      FileUtils.getFile("TmpD", ["webapps", aId, "application.zip"], true);
    let dir = this._getAppDir(aId);
    zipFile.moveTo(dir, "application.zip");
    let tmpDir = FileUtils.getDir("TmpD", ["webapps", aId], true, true);
    try {
      tmpDir.remove(true);
    } catch(e) { }

    
    let manFile = dir.clone();
    manFile.append("manifest.webapp");
    this._writeFile(manFile, JSON.stringify(aManifest), function() { });
    
    app.installState = "installed";
    app.downloading = false;
    app.downloadAvailable = false;
    this._saveApps((function() {
      this.updateAppHandlers(null, aManifest, aNewApp);
      this.broadcastMessage("Webapps:AddApp", { id: aId, app: aNewApp });
      Services.obs.notifyObservers(null, "webapps-installed",
        JSON.stringify({ manifestURL: aNewApp.manifestURL }));

      if (supportUseCurrentProfile()) {
        
        PermissionsInstaller.installPermissions({
          manifest: aManifest,
          origin: aNewApp.origin,
          manifestURL: aNewApp.manifestURL
        }, true);
      }

      this.updateDataStore(this.webapps[aId].localId, aNewApp.origin,
                           aNewApp.manifestURL, aManifest, aNewApp.appStatus);

      this.broadcastMessage("Webapps:UpdateState", {
        app: app,
        manifest: aManifest,
        manifestURL: aNewApp.manifestURL
      });
      this.broadcastMessage("Webapps:FireEvent", {
        eventType: ["downloadsuccess", "downloadapplied"],
        manifestURL: aNewApp.manifestURL
      });
      if (aInstallSuccessCallback) {
        aInstallSuccessCallback(aManifest, zipFile.path);
      }
    }).bind(this));
  },

  _nextLocalId: function() {
    let id = Services.prefs.getIntPref("dom.mozApps.maxLocalId") + 1;

    while (this.getManifestURLByLocalId(id)) {
      id++;
    }

    Services.prefs.setIntPref("dom.mozApps.maxLocalId", id);
    Services.prefs.savePrefFile(null);
    return id;
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
    this._writeFile(this.appsFile, JSON.stringify(this.webapps, null, 2),
                    aCallback);
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

    
    let baseDir = this.webapps[id].basePath == this.getCoreAppsBasePath()
                    ? "coreAppsDir" : DIRECTORY_NAME;
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

  downloadPackage: function(aManifest, aNewApp, aIsUpdate, aOnSuccess) {
    
    
    
    
    
    
    
    

    
    let id = this._appIdForManifestURL(aNewApp.manifestURL);
    let oldApp = this.webapps[id];

    return Task.spawn((function*() {
      yield this._ensureSufficientStorage(aNewApp);

      let fullPackagePath = aManifest.fullPackagePath();

      
      
      
      
      
      
      let isLocalFileInstall =
        Services.io.extractScheme(fullPackagePath) === 'file';

      debug("About to download " + fullPackagePath);

      let requestChannel = this._getRequestChannel(fullPackagePath,
                                                   isLocalFileInstall,
                                                   oldApp,
                                                   aNewApp);

      AppDownloadManager.add(
        aNewApp.manifestURL,
        {
          channel: requestChannel,
          appId: id,
          previousState: aIsUpdate ? "installed" : "pending"
        }
      );

      
      oldApp.downloading = true;

      
      
      
      oldApp.installState = aIsUpdate ? "updating" : "pending";

      
      oldApp.progress = 0;

      let zipFile = yield this._getPackage(requestChannel, id, oldApp, aNewApp);
      let hash = yield this._computeFileHash(zipFile.path);

      let responseStatus = requestChannel.responseStatus;
      let oldPackage = (responseStatus == 304 || hash == oldApp.packageHash);

      if (oldPackage) {
        debug("package's etag or hash unchanged; sending 'applied' event");
        
        
        this._sendAppliedEvent(aNewApp, oldApp, id);
        return;
      }

      let newManifest = yield this._openAndReadPackage(zipFile, oldApp, aNewApp,
              isLocalFileInstall, aIsUpdate, aManifest, requestChannel, hash);

      AppDownloadManager.remove(aNewApp.manifestURL);

      return [id, newManifest];

    }).bind(this)).then(
      aOnSuccess,
      this._revertDownloadPackage.bind(this, id, oldApp, aNewApp, aIsUpdate)
    );
  },

  _ensureSufficientStorage: function(aNewApp) {
    let deferred = Promise.defer();

    let navigator = Services.wm.getMostRecentWindow("navigator:browser")
                            .navigator;
    let deviceStorage = null;

    if (navigator.getDeviceStorage) {
      deviceStorage = navigator.getDeviceStorage("apps");
    }

    if (deviceStorage) {
      let req = deviceStorage.freeSpace();
      req.onsuccess = req.onerror = e => {
        let freeBytes = e.target.result;
        let sufficientStorage = this._checkDownloadSize(freeBytes, aNewApp);
        if (sufficientStorage) {
          deferred.resolve();
        } else {
          deferred.reject("INSUFFICIENT_STORAGE");
        }
      }
    } else {
      debug("No deviceStorage");
      
      
      let dir = FileUtils.getDir(DIRECTORY_NAME, ["webapps"], true, true);
      try {
        let sufficientStorage = this._checkDownloadSize(dir.diskSpaceAvailable,
                                                        aNewApp);
        if (sufficientStorage) {
          deferred.resolve();
        } else {
          deferred.reject("INSUFFICIENT_STORAGE");
        }
      } catch(ex) {
        
        
        
        
        deferred.resolve();
      }
    }

    return deferred.promise;
  },

  _checkDownloadSize: function(aFreeBytes, aNewApp) {
    if (aFreeBytes) {
      debug("Free storage: " + aFreeBytes + ". Download size: " +
            aNewApp.downloadSize);
      if (aFreeBytes <=
          aNewApp.downloadSize + AppDownloadManager.MIN_REMAINING_FREESPACE) {
        return false;
      }
    }
    return true;
  },

  _getRequestChannel: function(aFullPackagePath, aIsLocalFileInstall, aOldApp,
                               aNewApp) {
    let requestChannel;

    if (aIsLocalFileInstall) {
      requestChannel = NetUtil.newChannel(aFullPackagePath)
                              .QueryInterface(Ci.nsIFileChannel);
    } else {
      requestChannel = NetUtil.newChannel(aFullPackagePath)
                              .QueryInterface(Ci.nsIHttpChannel);
      requestChannel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;
    }

    if (aOldApp.packageEtag && !aIsLocalFileInstall) {
      debug("Add If-None-Match header: " + aOldApp.packageEtag);
      requestChannel.setRequestHeader("If-None-Match", aOldApp.packageEtag,
                                      false);
    }

    let lastProgressTime = 0;

    requestChannel.notificationCallbacks = {
      QueryInterface: function(aIID) {
        if (aIID.equals(Ci.nsISupports)          ||
            aIID.equals(Ci.nsIProgressEventSink) ||
            aIID.equals(Ci.nsILoadContext))
          return this;
        throw Cr.NS_ERROR_NO_INTERFACE;
      },
      getInterface: function(aIID) {
        return this.QueryInterface(aIID);
      },
      onProgress: (function(aRequest, aContext, aProgress, aProgressMax) {
        aOldApp.progress = aProgress;
        let now = Date.now();
        if (now - lastProgressTime > MIN_PROGRESS_EVENT_DELAY) {
          debug("onProgress: " + aProgress + "/" + aProgressMax);
          this._sendDownloadProgressEvent(aNewApp, aProgress);
          lastProgressTime = now;
          this._saveApps();
        }
      }).bind(this),
      onStatus: function(aRequest, aContext, aStatus, aStatusArg) { },

      
      appId: aOldApp.installerAppId,
      isInBrowserElement: aOldApp.installerIsBrowser,
      usePrivateBrowsing: false,
      isContent: false,
      associatedWindow: null,
      topWindow : null,
      isAppOfType: function(appType) {
        throw Cr.NS_ERROR_NOT_IMPLEMENTED;
      }
    };

    return requestChannel;
  },

  _sendDownloadProgressEvent: function(aNewApp, aProgress) {
    this.broadcastMessage("Webapps:UpdateState", {
      app: {
        progress: aProgress
      },
      manifestURL: aNewApp.manifestURL
    });
    this.broadcastMessage("Webapps:FireEvent", {
      eventType: "progress",
      manifestURL: aNewApp.manifestURL
    });
  },

  _getPackage: function(aRequestChannel, aId, aOldApp, aNewApp) {
    let deferred = Promise.defer();

    
    let zipFile =
      FileUtils.getFile("TmpD", ["webapps", aId, "application.zip"], true);

    
    let outputStream = Cc["@mozilla.org/network/file-output-stream;1"]
                         .createInstance(Ci.nsIFileOutputStream);
    
    outputStream.init(zipFile, 0x02 | 0x08 | 0x20, parseInt("0664", 8), 0);
    let bufferedOutputStream =
      Cc['@mozilla.org/network/buffered-output-stream;1']
        .createInstance(Ci.nsIBufferedOutputStream);
    bufferedOutputStream.init(outputStream, 1024);

    
    let listener = Cc["@mozilla.org/network/simple-stream-listener;1"]
                     .createInstance(Ci.nsISimpleStreamListener);

    listener.init(bufferedOutputStream, {
      onStartRequest: function(aRequest, aContext) {
        
      },

      onStopRequest: function(aRequest, aContext, aStatusCode) {
        bufferedOutputStream.close();
        outputStream.close();

        if (!Components.isSuccessCode(aStatusCode)) {
          deferred.reject("NETWORK_ERROR");
          return;
        }

        
        
        let responseStatus = aRequestChannel.responseStatus;
        if (responseStatus >= 400 && responseStatus <= 599) {
          
          aOldApp.downloadAvailable = false;
          deferred.reject("NETWORK_ERROR");
          return;
        }

        deferred.resolve(zipFile);
      }
    });
    aRequestChannel.asyncOpen(listener, null);

    
    this._sendDownloadProgressEvent(aNewApp, 0);

    return deferred.promise;
  },

  






  _computeFileHash: function(aFilePath) {
    return Task.spawn(function*() {
      let hasher = Cc["@mozilla.org/security/hash;1"]
                     .createInstance(Ci.nsICryptoHash);
      
      hasher.init(hasher.MD5);

      const CHUNK_SIZE = 16384;

      
      function toHexString(charCode) {
        return ("0" + charCode.toString(16)).slice(-2);
      }

      let file;
      try {
        file = yield OS.File.open(aFilePath, { read: true });
      } catch(e) {
        debug("Error opening " + aFilePath + ": " + e);
        return null;
      }

      try {
        let array;
        do {
          array = yield file.read(CHUNK_SIZE);
          hasher.update(array, array.length);
        } while (array.length == CHUNK_SIZE);
      } catch(e) {
        debug("Error reading " + aFilePath + ": " + e);
        return null;
      }

      yield file.close();

      
      let data = hasher.finish(false);
      
      let hash = [toHexString(data.charCodeAt(i)) for (i in data)].join("");
      debug("File hash computed: " + hash);

      return hash;
    });
  },

  











  _sendAppliedEvent: function(aNewApp, aOldApp, aId) {
    aOldApp.downloading = false;
    aOldApp.downloadAvailable = false;
    aOldApp.downloadSize = 0;
    aOldApp.installState = "installed";
    aOldApp.readyToApplyDownload = false;
    if (aOldApp.staged && aOldApp.staged.manifestHash) {
      
      
      
      aOldApp.manifestHash = aOldApp.staged.manifestHash;
      aOldApp.etag = aOldApp.staged.etag || aOldApp.etag;
      aOldApp.staged = {};
      
      let dirPath = this._getAppDir(aId).path;

      
      OS.File.move(OS.Path.join(dirPath, "staged-update.webapp"),
                   OS.Path.join(dirPath, "update.webapp"));
    }

    
    this._saveApps((function() {
      this.broadcastMessage("Webapps:UpdateState", {
        app: aOldApp,
        manifestURL: aNewApp.manifestURL
      });
      this.broadcastMessage("Webapps:FireEvent", {
        manifestURL: aNewApp.manifestURL,
        eventType: ["downloadsuccess", "downloadapplied"]
      });
    }).bind(this));
    let file = FileUtils.getFile("TmpD", ["webapps", aId], false);
    if (file && file.exists()) {
      file.remove(true);
    }
  },

  _openAndReadPackage: function(aZipFile, aOldApp, aNewApp, aIsLocalFileInstall,
                                aIsUpdate, aManifest, aRequestChannel, aHash) {
    return Task.spawn((function*() {
      let zipReader, isSigned, newManifest;

      try {
        [zipReader, isSigned] = yield this._openPackage(aZipFile, aOldApp,
                                                        aIsLocalFileInstall);
        newManifest = yield this._readPackage(aOldApp, aNewApp,
                aIsLocalFileInstall, aIsUpdate, aManifest, aRequestChannel,
                aHash, zipReader, isSigned);
      } catch (e) {
        debug("package open/read error: " + e);
        
        
        
        
        
        if (aOldApp.installState !== "pending") {
          aOldApp.downloadAvailable = false;
        }
        if (typeof e == 'object') {
          Cu.reportError("Error while reading package:" + e);
          throw "INVALID_PACKAGE";
        } else {
          throw e;
        }
      } finally {
        if (zipReader) {
          zipReader.close();
        }
      }

      return newManifest;

    }).bind(this));
  },

  _openPackage: function(aZipFile, aApp, aIsLocalFileInstall) {
    return Task.spawn((function*() {
      let certDb;
      try {
        certDb = Cc["@mozilla.org/security/x509certdb;1"]
                   .getService(Ci.nsIX509CertDB);
      } catch (e) {
        debug("nsIX509CertDB error: " + e);
        
        aApp.downloadAvailable = false;
        throw "CERTDB_ERROR";
      }

      let [result, zipReader] = yield this._openSignedPackage(aZipFile, certDb);

      
      
      
      
      let isLaterThanBuildTime = Date.now() > PLATFORM_BUILD_ID_TIME;

      let isSigned;

      if (Components.isSuccessCode(result)) {
        isSigned = true;
      } else if (result == Cr.NS_ERROR_SIGNED_JAR_MODIFIED_ENTRY ||
                 result == Cr.NS_ERROR_SIGNED_JAR_UNSIGNED_ENTRY ||
                 result == Cr.NS_ERROR_SIGNED_JAR_ENTRY_MISSING) {
        throw "APP_PACKAGE_CORRUPTED";
      } else if (result == Cr.NS_ERROR_FILE_CORRUPTED ||
                 result == Cr.NS_ERROR_SIGNED_JAR_ENTRY_TOO_LARGE ||
                 result == Cr.NS_ERROR_SIGNED_JAR_ENTRY_INVALID ||
                 result == Cr.NS_ERROR_SIGNED_JAR_MANIFEST_INVALID) {
        throw "APP_PACKAGE_INVALID";
      } else if ((!aIsLocalFileInstall || isLaterThanBuildTime) &&
                 (result != Cr.NS_ERROR_SIGNED_JAR_NOT_SIGNED)) {
        throw "INVALID_SIGNATURE";
      } else {
        
        
        
        
        isSigned = (aIsLocalFileInstall &&
                    (getNSPRErrorCode(result) ==
                     SEC_ERROR_EXPIRED_CERTIFICATE));

        zipReader = Cc["@mozilla.org/libjar/zip-reader;1"]
                      .createInstance(Ci.nsIZipReader);
        zipReader.open(aZipFile);
      }

      return [zipReader, isSigned];

    }).bind(this));
  },

  _openSignedPackage: function(aZipFile, aCertDb) {
    let deferred = Promise.defer();

    aCertDb.openSignedJARFileAsync(
       aZipFile,
       function(aRv, aZipReader) {
         deferred.resolve([aRv, aZipReader]);
       }
    );

    return deferred.promise;
  },

  _readPackage: function(aOldApp, aNewApp, aIsLocalFileInstall, aIsUpdate,
                         aManifest, aRequestChannel, aHash, aZipReader,
                         aIsSigned) {
    this._checkSignature(aNewApp, aIsSigned, aIsLocalFileInstall);

    if (!aZipReader.hasEntry("manifest.webapp")) {
      throw "MISSING_MANIFEST";
    }

    let istream = aZipReader.getInputStream("manifest.webapp");

    
    let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                      .createInstance(Ci.nsIScriptableUnicodeConverter);
    converter.charset = "UTF-8";

    let newManifest = JSON.parse(converter.ConvertToUnicode(
          NetUtil.readInputStreamToString(istream, istream.available()) || ""));

    if (!AppsUtils.checkManifest(newManifest, aOldApp)) {
      throw "INVALID_MANIFEST";
    }

    
    
    
    
    if (aIsUpdate) {
      
      
      AppsUtils.ensureSameAppName(aManifest._manifest, newManifest, aOldApp);
    }

    if (!AppsUtils.compareManifests(newManifest, aManifest._manifest)) {
      throw "MANIFEST_MISMATCH";
    }

    if (!AppsUtils.checkInstallAllowed(newManifest, aNewApp.installOrigin)) {
      throw "INSTALL_FROM_DENIED";
    }

    
    let maxStatus = aIsSigned || aIsLocalFileInstall
                    ? Ci.nsIPrincipal.APP_STATUS_PRIVILEGED
                    : Ci.nsIPrincipal.APP_STATUS_INSTALLED;

    if (AppsUtils.getAppManifestStatus(newManifest) > maxStatus) {
      throw "INVALID_SECURITY_LEVEL";
    }

    aOldApp.appStatus = AppsUtils.getAppManifestStatus(newManifest);

    this._saveEtag(aIsUpdate, aOldApp, aRequestChannel, aHash, newManifest);
    this._checkOrigin(aIsSigned, aOldApp, newManifest, aIsUpdate);
    this._getIds(aIsSigned, aZipReader, converter, aNewApp, aOldApp, aIsUpdate);

    return newManifest;
  },

  _checkSignature: function(aApp, aIsSigned, aIsLocalFileInstall) {
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    
    let signedAppOriginsStr =
      Services.prefs.getCharPref("dom.mozApps.signed_apps_installable_from");
    
    
    let isSignedAppOrigin = (aIsSigned && aIsLocalFileInstall) ||
                             signedAppOriginsStr.split(",").
                                   indexOf(aApp.installOrigin) > -1;
    if (!aIsSigned && isSignedAppOrigin) {
      
      
      throw "INVALID_SIGNATURE";
    } else if (aIsSigned && !isSignedAppOrigin) {
      
      
      
      
      
      throw "INSTALL_FROM_DENIED";
    }
  },

  _saveEtag: function(aIsUpdate, aOldApp, aRequestChannel, aHash, aManifest) {
    
    if (aIsUpdate) {
      if (!aOldApp.staged) {
        aOldApp.staged = { };
      }
      try {
        aOldApp.staged.packageEtag = aRequestChannel.getResponseHeader("Etag");
      } catch(e) { }
      aOldApp.staged.packageHash = aHash;
      aOldApp.staged.appStatus = AppsUtils.getAppManifestStatus(aManifest);
    } else {
      try {
        aOldApp.packageEtag = aRequestChannel.getResponseHeader("Etag");
      } catch(e) { }
      aOldApp.packageHash = aHash;
      aOldApp.appStatus = AppsUtils.getAppManifestStatus(aManifest);
    }
  },

  _checkOrigin: function(aIsSigned, aOldApp, aManifest, aIsUpdate) {
    
    if (aIsSigned &&
        aOldApp.appStatus >= Ci.nsIPrincipal.APP_STATUS_PRIVILEGED &&
        aManifest.origin !== undefined) {
      let uri;
      try {
        uri = Services.io.newURI(aManifest.origin, null, null);
      } catch(e) {
        throw "INVALID_ORIGIN";
      }
      if (uri.scheme != "app") {
        throw "INVALID_ORIGIN";
      }

      if (aIsUpdate) {
        
        if (uri.prePath != app.origin) {
          throw "INVALID_ORIGIN_CHANGE";
        }
        
        
        
      } else {
        debug("Setting origin to " + uri.prePath +
              " for " + app.manifestURL);
        let newId = uri.prePath.substring(6); 
        if (newId in this.webapps) {
          throw "DUPLICATE_ORIGIN";
        }
        aOldApp.origin = uri.prePath;
        
        aOldApp.id = newId;
        this.webapps[newId] = aOldApp;
        delete this.webapps[aId];
        
        [DIRECTORY_NAME, "TmpD"].forEach(function(aDir) {
          let parent = FileUtils.getDir(aDir, ["webapps"], true, true);
          let dir = FileUtils.getDir(aDir, ["webapps", aId], true, true);
          dir.moveTo(parent, newId);
        });
        
        this.broadcastMessage("Webapps:RemoveApp", { id: aId });
        this.broadcastMessage("Webapps:AddApp", { id: newId,
                                                  app: aOldApp });
      }
    }
  },

  _getIds: function(aIsSigned, aZipReader, aConverter, aNewApp, aOldApp,
                    aIsUpdate) {
    
    if (aIsSigned) {
      let idsStream;
      try {
        idsStream = aZipReader.getInputStream("META-INF/ids.json");
      } catch (e) {
        throw aZipReader.hasEntry("META-INF/ids.json")
               ? e
               : "MISSING_IDS_JSON";
      }

      let ids = JSON.parse(aConverter.ConvertToUnicode(NetUtil.
             readInputStreamToString( idsStream, idsStream.available()) || ""));
      if ((!ids.id) || !Number.isInteger(ids.version) ||
          (ids.version <= 0)) {
         throw "INVALID_IDS_JSON";
      }
      let storeId = aNewApp.installOrigin + "#" + ids.id;
      this._checkForStoreIdMatch(aIsUpdate, aOldApp, storeId, ids.version);
      aOldApp.storeId = storeId;
      aOldApp.storeVersion = ids.version;
    }
  },

  
  
  
  _checkForStoreIdMatch: function(aIsUpdate, aNewApp, aStoreId, aStoreVersion) {
    
    
    
    
    
    
    
    
    
    
    
    

    
    let appId = this.getAppLocalIdByStoreId(aStoreId);
    let isInstalled = appId != Ci.nsIScriptSecurityManager.NO_APP_ID;
    if (aIsUpdate) {
      let isDifferent = aNewApp.localId !== appId;
      let isPending = aNewApp.storeId.indexOf(STORE_ID_PENDING_PREFIX) == 0;

      if ((!isInstalled && !isPending) || (isInstalled && isDifferent)) {
        throw "WRONG_APP_STORE_ID";
      }

      if (!isPending && (aNewApp.storeVersion >= aStoreVersion)) {
        throw "APP_STORE_VERSION_ROLLBACK";
      }

    } else if (isInstalled) {
      throw "WRONG_APP_STORE_ID";
    }
  },

  
  _revertDownloadPackage: function(aId, aOldApp, aNewApp, aIsUpdate, aError) {
    debug("Cleanup: " + aError + "\n" + aError.stack);
    let dir = FileUtils.getDir("TmpD", ["webapps", aId], true, true);
    try {
      dir.remove(true);
    } catch (e) { }

    
    
    
    if (aOldApp.isCanceling) {
      delete aOldApp.isCanceling;
      return;
    }

    let download = AppDownloadManager.get(aNewApp.manifestURL);
    aOldApp.downloading = false;

    
    
    
    
    aOldApp.installState = download ? download.previousState
                                    : aIsUpdate ? "installed"
                                                : "pending";

    if (aOldApp.staged) {
      delete aOldApp.staged;
    }

    this._saveApps((function() {
      this.broadcastMessage("Webapps:UpdateState", {
        app: aOldApp,
        error: aError,
        manifestURL: aNewApp.manifestURL
      });
      this.broadcastMessage("Webapps:FireEvent", {
        eventType: "downloaderror",
        manifestURL:  aNewApp.manifestURL
      });
    }).bind(this));
    AppDownloadManager.remove(aNewApp.manifestURL);
  },

  doUninstall: function(aData, aMm) {
    this.uninstall(aData.manifestURL,
      function onsuccess() {
        aMm.sendAsyncMessage("Webapps:Uninstall:Return:OK", aData);
      },
      function onfailure() {
        
        
        
        aMm.sendAsyncMessage("Webapps:Uninstall:Return:KO", aData);
      }
    );
  },

  uninstall: function(aManifestURL, aOnSuccess, aOnFailure) {
    debug("uninstall " + aManifestURL);

    let app = this.getAppByManifestURL(aManifestURL);
    if (!app) {
      aOnFailure("NO_SUCH_APP");
      return;
    }
    let id = app.id;

    if (!app.removable) {
      debug("Error: cannot uninstall a non-removable app.");
      aOnFailure("NON_REMOVABLE_APP");
      return;
    }

    
    
    this.cancelDownload(app.manifestURL);

    
    if (id in this._manifestCache) {
      delete this._manifestCache[id];
    }

    
    this._clearPrivateData(app.localId, false);

    
    
    
    let appClone = AppsUtils.cloneAppObject(app);
    Services.obs.notifyObservers(null, "webapps-uninstall", JSON.stringify(appClone));

    if (supportSystemMessages()) {
      this._readManifests([{ id: id }], (function unregisterManifest(aResult) {
        this._unregisterActivities(aResult[0].manifest, app);
      }).bind(this));
    }

    let dir = this._getAppDir(id);
    try {
      dir.remove(true);
    } catch (e) {}

    delete this.webapps[id];

    this._saveApps((function() {
      this.broadcastMessage("Webapps:Uninstall:Broadcast:Return:OK", appClone);
      
      try {
        if (aOnSuccess) {
          aOnSuccess();
        }
      } catch(ex) {
        Cu.reportError("DOMApplicationRegistry: Exception on app uninstall: " +
                       ex + "\n" + ex.stack);
      }
      this.broadcastMessage("Webapps:RemoveApp", { id: id });
    }).bind(this));
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
          this._isLaunchable(this.webapps[id])) {
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
      if (this.webapps[appId].manifestURL == aData.manifestURL &&
          this._isLaunchable(this.webapps[appId])) {
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
          this._isLaunchable(this.webapps[id])) {
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
      if (!this._isLaunchable(this.webapps[id])) {
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

  doGetAll: function(aData, aMm) {
    this.getAll(function (apps) {
      aData.apps = apps;
      aMm.sendAsyncMessage("Webapps:GetAll:Return:OK", aData);
    });
  },

  getAll: function(aCallback) {
    debug("getAll");
    let apps = [];
    let tmp = [];

    for (let id in this.webapps) {
      let app = AppsUtils.cloneAppObject(this.webapps[id]);
      if (!this._isLaunchable(app))
        continue;

      apps.push(app);
      tmp.push({ id: id });
    }

    this._readManifests(tmp, (function(aResult) {
      for (let i = 0; i < aResult.length; i++)
        apps[i].manifest = aResult[i].manifest;
      aCallback(apps);
    }).bind(this));
  },

  getManifestFor: function(aManifestURL, aCallback) {
    if (!aCallback)
      return;

    let id = this._appIdForManifestURL(aManifestURL);
    let app = this.webapps[id];
    if (!id || (app.installState == "pending" && !app.retryingDownload)) {
      aCallback(null);
      return;
    }

    this._readManifests([{ id: id }], function(aResult) {
      aCallback(aResult[0].manifest);
    });
  },

  getAppByManifestURL: function(aManifestURL) {
    return AppsUtils.getAppByManifestURL(this.webapps, aManifestURL);
  },

  getCSPByLocalId: function(aLocalId) {
    debug("getCSPByLocalId:" + aLocalId);
    return AppsUtils.getCSPByLocalId(this.webapps, aLocalId);
  },

  getAppLocalIdByStoreId: function(aStoreId) {
    debug("getAppLocalIdByStoreId:" + aStoreId);
    return AppsUtils.getAppLocalIdByStoreId(this.webapps, aStoreId);
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

  getCoreAppsBasePath: function() {
    return AppsUtils.getCoreAppsBasePath();
  },

  getWebAppsBasePath: function getWebAppsBasePath() {
    return FileUtils.getDir(DIRECTORY_NAME, ["webapps"], false).path;
  },

  _isLaunchable: function(aApp) {
    if (this.allAppsLaunchable)
      return true;

    return WebappOSUtils.isLaunchable(aApp);
  },

  _notifyCategoryAndObservers: function(subject, topic, data,  msg) {
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
    
    if (msg) {
      ppmm.broadcastAsyncMessage("Webapps:ClearBrowserData:Return", msg);
    }
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
        this._clearPrivateData(appId, true, message.data);
        break;
    }
  },

  _clearPrivateData: function(appId, browserOnly, msg) {
    let subject = {
      appId: appId,
      browserOnly: browserOnly,
      QueryInterface: XPCOMUtils.generateQI([Ci.mozIApplicationClearPrivateDataParams])
    };
    this._notifyCategoryAndObservers(subject, "webapps-clear-data", null, msg);
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
    DOMApplicationRegistry.broadcastMessage("Webapps:UpdateState", {
      app: app,
      manifestURL: app.manifestURL
    });
    DOMApplicationRegistry.broadcastMessage("Webapps:FireEvent", {
      eventType: "progress",
      manifestURL: app.manifestURL
    });
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
      if (aStatus != "installed") {
        self._sendProgressEvent();
        return;
      }

      app.updateTime = Date.now();
      app.downloading = false;
      app.downloadAvailable = false;
      DOMApplicationRegistry.broadcastMessage("Webapps:UpdateState", {
        app: app,
        manifestURL: app.manifestURL
      });
      DOMApplicationRegistry.broadcastMessage("Webapps:FireEvent", {
        eventType: ["downloadsuccess", "downloadapplied"],
        manifestURL: app.manifestURL
      });
    }

    let setError = function appObs_setError(aError) {
      debug("Offlinecache setError to " + aError);
      app.downloading = false;
      DOMApplicationRegistry.broadcastMessage("Webapps:UpdateState", {
        app: app,
        manifestURL: app.manifestURL
      });
      DOMApplicationRegistry.broadcastMessage("Webapps:FireEvent", {
        error: aError,
        eventType: "downloaderror",
        manifestURL: app.manifestURL
      });
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
        setStatus(this.startStatus, aUpdate.byteProgress);
        break;
      case Ci.nsIOfflineCacheUpdateObserver.STATE_ITEMSTARTED:
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
