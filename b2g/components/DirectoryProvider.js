



const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/XPCOMUtils.jsm");
Cu.import("resource://gre/modules/Services.jsm");

const XRE_OS_UPDATE_APPLY_TO_DIR = "OSUpdApplyToD"
const UPDATE_ARCHIVE_DIR = "UpdArchD"
const LOCAL_DIR = "/data/local";

XPCOMUtils.defineLazyServiceGetter(Services, "env",
                                   "@mozilla.org/process/environment;1",
                                   "nsIEnvironment");

XPCOMUtils.defineLazyServiceGetter(Services, "um",
                                   "@mozilla.org/updates/update-manager;1",
                                   "nsIUpdateManager");

XPCOMUtils.defineLazyServiceGetter(Services, "volumeService",
                                   "@mozilla.org/telephony/volume-service;1",
                                   "nsIVolumeService");

XPCOMUtils.defineLazyGetter(this, "gExtStorage", function dp_gExtStorage() {
    return Services.env.get("EXTERNAL_STORAGE");
});


const gUseSDCard = true;

const VERBOSE = 1;
let log =
  VERBOSE ?
  function log_dump(msg) { dump("DirectoryProvider: " + msg + "\n"); } :
  function log_noop(msg) { };

function DirectoryProvider() {
}

DirectoryProvider.prototype = {
  classID: Components.ID("{9181eb7c-6f87-11e1-90b1-4f59d80dd2e5}"),

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIDirectoryServiceProvider]),

  getFile: function dp_getFile(prop, persistent) {
#ifdef MOZ_WIDGET_GONK
    let localProps = ["cachePDir", "webappsDir", "PrefD", "indexedDBPDir",
                      "permissionDBPDir", "UpdRootD"];
    if (localProps.indexOf(prop) != -1) {
      let file = Cc["@mozilla.org/file/local;1"]
                   .createInstance(Ci.nsILocalFile)
      file.initWithPath(LOCAL_DIR);
      persistent.value = true;
      return file;
    }
    if (prop == "coreAppsDir") {
      let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile)
      file.initWithPath("/system/b2g");
      persistent.value = true;
      return file;
    }
    if (prop == XRE_OS_UPDATE_APPLY_TO_DIR ||
        prop == UPDATE_ARCHIVE_DIR) {
      let file = this.getUpdateDir(persistent);
      return file;
    }
#endif
    return null;
  },

  
  volumeHasFreeSpace: function dp_volumeHasFreeSpace(volumePath, requiredSpace) {
    if (!volumePath) {
      return false;
    }
    if (!Services.volumeService) {
      return false;
    }
    let volume = Services.volumeService.getVolumeByPath(volumePath);
    if (!volume || volume.state !== Ci.nsIVolume.STATE_MOUNTED) {
      return false;
    }
    let stat = volume.getStats();
    if (!stat) {
      return false;
    }
    return requiredSpace <= stat.freeBytes;
  },

  findUpdateDirWithFreeSpace: function dp_findUpdateDirWithFreeSpace(requiredSpace) {
    if (!Services.volumeService) {
      return this.createUpdatesDir(LOCAL_DIR);
    }
    let activeUpdate = Services.um.activeUpdate;
    if (gUseSDCard) {
      if (this.volumeHasFreeSpace(gExtStorage, requiredSpace)) {
        let extUpdateDir = this.createUpdatesDir(gExtStorage);
        if (extUpdateDir !== null) {
          return extUpdateDir;
        }
        log("Warning: " + gExtStorage + " has enough free space for update " +
            activeUpdate.name + ", but is not writable");
      }
    }

    if (this.volumeHasFreeSpace(LOCAL_DIR, requiredSpace)) {
      let localUpdateDir = this.createUpdatesDir(LOCAL_DIR);
      if (localUpdateDir !== null) {
        return localUpdateDir;
      }
      log("Warning: " + LOCAL_DIR + " has enough free space for update " +
          activeUpdate.name + ", but is not writable");
    }

    return null;
  },

  getUpdateDir: function dp_getUpdateDir(persistent) {
    let defaultUpdateDir = this.getDefaultUpdateDir();
    persistent.value = false;

    let activeUpdate = Services.um.activeUpdate;
    if (!activeUpdate) {
      log("Warning: No active update found, using default update dir: " +
          defaultUpdateDir);
      return defaultUpdateDir;
    }

    let selectedPatch = activeUpdate.selectedPatch;
    if (!selectedPatch) {
      log("Warning: No selected patch, using default update dir: " +
          defaultUpdateDir);
      return defaultUpdateDir;
    }

    let requiredSpace = selectedPatch.size * 2;
    let updateDir = this.findUpdateDirWithFreeSpace(requiredSpace, persistent);
    if (updateDir) {
      return updateDir;
    }

    
    
    
    log("Error: No volume found with " + requiredSpace + " bytes for downloading"+
        " update " + activeUpdate.name);
    throw Cr.NS_ERROR_FILE_TOO_BIG;
  },

  createUpdatesDir: function dp_createUpdatesDir(root) {
      let dir = Cc["@mozilla.org/file/local;1"]
                   .createInstance(Ci.nsILocalFile);
      dir.initWithPath(root);
      if (!dir.isWritable()) {
        return null;
      }
      dir.appendRelativePath("updates/0");
      if (dir.exists()) {
        if (dir.isDirectory() && dir.isWritable()) {
          return dir;
        }
        
        
        log("Error: " + dir.path + " is a file or isn't writable");
        return null;
      }
      
      
      try {
        dir.create(Ci.nsIFile.DIRECTORY_TYPE, 0770);
      } catch (e) {
        
        log("Error: " + dir.path + " unable to create directory");
        return null;
      }
      return dir;
  },

  getDefaultUpdateDir: function dp_getDefaultUpdateDir() {
    let path = gExtStorage;
    if (!path) {
      path = LOCAL_DIR;
    }

    if (Services.volumeService) {
      let extVolume = Services.volumeService.getVolumeByPath(path);
      if (!extVolume) {
        path = LOCAL_DIR;
      }
    }

    let dir = Cc["@mozilla.org/file/local;1"]
                 .createInstance(Ci.nsILocalFile)
    dir.initWithPath(path);

    if (!dir.exists() && path != LOCAL_DIR) {
      
      dir.initWithPath(LOCAL_DIR);

      if (!dir.exists()) {
        throw Cr.NS_ERROR_FILE_NOT_FOUND;
      }
    }

    dir.appendRelativePath("updates");
    return dir;
  }
};

this.NSGetFactory = XPCOMUtils.generateNSGetFactory([DirectoryProvider]);
