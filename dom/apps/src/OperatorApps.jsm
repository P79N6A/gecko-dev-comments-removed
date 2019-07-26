



"use strict";

const Cu = Components.utils;
const Cc = Components.classes;
const Ci = Components.interfaces;

this.EXPORTED_SYMBOLS = ["OperatorAppsRegistry"];

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/Webapps.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");

let Path = OS.Path;

#ifdef MOZ_B2G_RIL
XPCOMUtils.defineLazyServiceGetter(this, "iccProvider",
                                   "@mozilla.org/ril/content-helper;1",
                                   "nsIIccProvider");
#endif

function debug(aMsg) {
  
}









const DIRECTORY_NAME = "webappsDir";
const SINGLE_VARIANT_SOURCE_DIR = "svoperapps";
const SINGLE_VARIANT_CONF_FILE  = "singlevariantconf.json";
const PREF_FIRST_RUN_WITH_SIM   = "dom.webapps.firstRunWithSIM";
const PREF_SINGLE_VARIANT_DIR   = "dom.mozApps.single_variant_sourcedir";
const METADATA                  = "metadata.json";
const UPDATEMANIFEST            = "update.webapp";
const MANIFEST                  = "manifest.webapp";
const APPLICATION_ZIP           = "application.zip";

function isFirstRunWithSIM() {
  try {
    if (Services.prefs.prefHasUserValue(PREF_FIRST_RUN_WITH_SIM)) {
      return Services.prefs.getBoolPref(PREF_FIRST_RUN_WITH_SIM);
    }
  } catch(e) {
    debug ("Error getting pref. " + e);
  }
  return true;
}

#ifdef MOZ_B2G_RIL
let iccListener = {
  notifyStkCommand: function() {},

  notifyStkSessionEnd: function() {},

  notifyCardStateChanged: function() {},

  notifyIccInfoChanged: function() {
    
    
    
    
    
    let clientId = 0;
    let iccInfo = iccProvider.getIccInfo(clientId);
    if (iccInfo && iccInfo.mcc && iccInfo.mnc) {
      let mcc = iccInfo.mcc;
      let mnc = iccInfo.mnc;
      debug("******* iccListener cardIccInfo MCC-MNC: " + mcc + "-" + mnc);
      iccProvider.unregisterIccMsg(clientId, this);
      OperatorAppsRegistry._installOperatorApps(mcc, mnc);

      debug("Broadcast message first-run-with-sim");
      let messenger = Cc["@mozilla.org/system-message-internal;1"]
                        .getService(Ci.nsISystemMessagesInternal);
      messenger.broadcastMessage("first-run-with-sim", { mcc: mcc,
                                                         mnc: mnc });
    }
  }
};
#endif

this.OperatorAppsRegistry = {

  _baseDirectory: null,

  init: function() {
    debug("init");
#ifdef MOZ_B2G_RIL
    if (isFirstRunWithSIM()) {
      debug("First Run with SIM");
      Task.spawn(function() {
        try {
          yield this._initializeSourceDir();
          
          
          
          
          
          
          let clientId = 0;
          let iccInfo = iccProvider.getIccInfo(clientId);
          let mcc = 0;
          let mnc = 0;
          if (iccInfo && iccInfo.mcc) {
            mcc = iccInfo.mcc;
          }
          if (iccInfo && iccInfo.mnc) {
            mnc = iccInfo.mnc;
          }
          if (mcc && mnc) {
            this._installOperatorApps(mcc, mnc);
          } else {
            iccProvider.registerIccMsg(clientId, iccListener);
          }
        } catch (e) {
          debug("Error Initializing OperatorApps. " + e);
        }
      }.bind(this));
    } else {
      debug("No First Run with SIM");
    }
#endif
  },

  _copyDirectory: function(aOrg, aDst) {
    debug("copying " + aOrg + " to " + aDst);
    return aDst && Task.spawn(function() {
      try {
        let orgDir = Cc["@mozilla.org/file/local;1"]
                       .createInstance(Ci.nsIFile);
        orgDir.initWithPath(aOrg);
        if (!orgDir.exists() || !orgDir.isDirectory()) {
          debug(aOrg + " does not exist or is not a directory");
          return;
        }

        let dstDir = Cc["@mozilla.org/file/local;1"]
                       .createInstance(Ci.nsIFile);
        dstDir.initWithPath(aDst);
        if (!dstDir.exists()) {
          dstDir.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
        }

        let entries = orgDir.directoryEntries;
        while (entries.hasMoreElements()) {
          let entry = entries.getNext().QueryInterface(Ci.nsIFile);

          if (!entry.isDirectory()) {
            
            let dstFile = dstDir.clone();
            dstFile.append(entry.leafName);
            if(dstFile.exists()) {
              dstFile.remove(false);
            }

            entry.copyTo(dstDir, entry.leafName);
          } else {
            yield this._copyDirectory(entry.path,
                                      Path.join(aDst, entry.leafName));
          }
        }
      } catch (e) {
        debug("Error copying " + aOrg + " to " + aDst + ". " + e);
      }
    }.bind(this));
  },

  _initializeSourceDir: function() {
    return Task.spawn(function() {
      let svFinalDirName;
      try {
        svFinalDirName = Services.prefs.getCharPref(PREF_SINGLE_VARIANT_DIR);
      } catch(e) {
        debug ("Error getting pref. " + e);
        this.appsDir = FileUtils.getFile(DIRECTORY_NAME,
                                         [SINGLE_VARIANT_SOURCE_DIR]).path;
        return;
      }
      
      
      
      
      
      let svFinalDir = Cc["@mozilla.org/file/local;1"]
                         .createInstance(Ci.nsIFile);
      svFinalDir.initWithPath(svFinalDirName);
      if (!svFinalDir.exists()) {
        svFinalDir.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
      }

      let svIndex = svFinalDir.clone();
      svIndex.append(SINGLE_VARIANT_CONF_FILE);
      if (!svIndex.exists()) {
        let svSourceDir = FileUtils.getFile(DIRECTORY_NAME,
                                            [SINGLE_VARIANT_SOURCE_DIR]);

        yield this._copyDirectory(svSourceDir.path, svFinalDirName);

        debug("removing directory:" + svSourceDir.path);
        try {
          svSourceDir.remove(true);
        } catch(ex) { }
      }

      this.appsDir = svFinalDirName;
    }.bind(this));
  },

  set appsDir(aDir) {
    debug("appsDir SET: " + aDir);
    if (aDir) {
      this._baseDirectory = Cc["@mozilla.org/file/local;1"]
          .createInstance(Ci.nsILocalFile);
      this._baseDirectory.initWithPath(aDir);
    } else {
      this._baseDirectory = null;
    }
  },

  get appsDir() {
    return this._baseDirectory;
  },

  eraseVariantAppsNotInList: function(aIdsApp) {
    if (!aIdsApp || !Array.isArray(aIdsApp)) {
      aIdsApp = [ ];
    }

    let svDir;
    try {
      svDir = this.appsDir.clone();
    } catch (e) {
      debug("eraseVariantAppsNotInList --> Error getting Dir "+
             svDir.path + ". " + e);
      return;
    }

    if (!svDir || !svDir.exists()) {
      return;
    }

    let entries = svDir.directoryEntries;
    while (entries.hasMoreElements()) {
      let entry = entries.getNext().QueryInterface(Ci.nsIFile);
      if (entry.isDirectory() && aIdsApp.indexOf(entry.leafName) < 0) {
        try{
          entry.remove(true);
        } catch(e) {
          debug("Error removing [" + entry.path + "]." + e);
        }
      }
    }
  },

  _launchInstall: function(isPackage, aId, aMetadata, aManifest) {
    if (!aManifest) {
      debug("Error: The application " + aId + " does not have a manifest");
      return;
    }

    let appData = {
      app: {
        installOrigin: aMetadata.installOrigin,
        origin: aMetadata.origin,
        manifestURL: aMetadata.manifestURL,
        manifestHash: AppsUtils.computeHash(JSON.stringify(aManifest))
      },
      appId: undefined,
      isBrowser: false,
      isPackage: isPackage,
      forceSuccessAck: true
    };

    if (isPackage) {
      debug("aId:" + aId + ". Installing as packaged app.");
      let installPack = this.appsDir.clone();
      installPack.append(aId);
      installPack.append(APPLICATION_ZIP);

      if (!installPack.exists()) {
        debug("SV " + installPack.path + " file do not exists for app " + aId);
        return;
      }

      appData.app.localInstallPath = installPack.path;
      appData.app.updateManifest = aManifest;
      DOMApplicationRegistry.confirmInstall(appData);
    } else {
      debug("aId:" + aId + ". Installing as hosted app.");
      appData.app.manifest = aManifest;
      DOMApplicationRegistry.confirmInstall(appData);
    }
  },

  _installOperatorApps: function(aMcc, aMnc) {
    Task.spawn(function() {
      debug("Install operator apps ---> mcc:"+ aMcc + ", mnc:" + aMnc);
      if (!isFirstRunWithSIM()) {
        debug("Operator apps already installed.");
        return;
      }

      let aIdsApp = yield this._getSingleVariantApps(aMcc, aMnc);
      debug("installOperatorApps --> aIdsApp:" + JSON.stringify(aIdsApp));
      for (let i = 0; i < aIdsApp.length; i++) {
        let aId = aIdsApp[i];
        let aMetadata = yield AppsUtils.loadJSONAsync(
                           Path.join(this.appsDir.path, aId, METADATA));
        if (!aMetadata) {
          debug("Error reading metadata file");
          return;
        }

        debug("metadata:" + JSON.stringify(aMetadata));
        let isPackage = true;
        let manifest;
        let manifests = [UPDATEMANIFEST, MANIFEST];
        for (let j = 0; j < manifests.length; j++) {
          manifest = yield AppsUtils.loadJSONAsync(
                        Path.join(this.appsDir.path, aId, manifests[j]));

          if (!manifest) {
            isPackage = false;
          } else {
            break;
          }
        }
        if (manifest) {
          this._launchInstall(isPackage, aId, aMetadata, manifest);
        } else {
          debug ("Error. Neither " + UPDATEMANIFEST + " file nor " + MANIFEST +
                 " file for " + aId + " app.");
        }
      }
      this.eraseVariantAppsNotInList(aIdsApp);
      Services.prefs.setBoolPref(PREF_FIRST_RUN_WITH_SIM, false);
      Services.prefs.savePrefFile(null);
    }.bind(this)).then(null, function(aError) {
        debug("Error: " + aError);
    });
  },

  _getSingleVariantApps: function(aMcc, aMnc) {

    function normalizeCode(aCode) {
      let ncode = "" + aCode;
      while (ncode.length < 3) {
        ncode = "0" + ncode;
      }
      return ncode;
    }

    return Task.spawn(function*() {
      let key = normalizeCode(aMcc) + "-" + normalizeCode(aMnc);
      let file = Path.join(this.appsDir.path, SINGLE_VARIANT_CONF_FILE);
      let aData = yield AppsUtils.loadJSONAsync(file);

      if (!aData || !(key in aData)) {
        return [];
      }

      return aData[key];
    }.bind(this));
  }
};

OperatorAppsRegistry.init();
