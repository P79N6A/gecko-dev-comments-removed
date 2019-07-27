



this.EXPORTED_SYMBOLS = ["NativeApp"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/WebappOSUtils.jsm");
Cu.import("resource://gre/modules/AppsUtils.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

const DEFAULT_ICON_URL = "chrome://global/skin/icons/webapps-64.png";

const ERR_NOT_INSTALLED = "The application isn't installed";
const ERR_UPDATES_UNSUPPORTED_OLD_NAMING_SCHEME =
  "Updates for apps installed with the old naming scheme unsupported";


const PERMS_DIRECTORY = OS.Constants.libc.S_IRWXU |
                        OS.Constants.libc.S_IRGRP | OS.Constants.libc.S_IXGRP |
                        OS.Constants.libc.S_IROTH | OS.Constants.libc.S_IXOTH;


const PERMS_FILE = OS.Constants.libc.S_IRUSR | OS.Constants.libc.S_IWUSR |
                   OS.Constants.libc.S_IRGRP |
                   OS.Constants.libc.S_IROTH;

const DESKTOP_DIR = OS.Constants.Path.desktopDir;
const HOME_DIR = OS.Constants.Path.homeDir;
const TMP_DIR = OS.Constants.Path.tmpDir;














function CommonNativeApp(aApp, aManifest, aCategories, aRegistryDir) {
  
  
  aApp.name = aManifest.name;
  this.uniqueName = WebappOSUtils.getUniqueName(aApp);

  let localeManifest =
    new ManifestHelper(aManifest, aApp.origin, aApp.manifestURL);

  this.appLocalizedName = localeManifest.name;
  this.appNameAsFilename = stripStringForFilename(aApp.name);

  if (aApp.updateManifest) {
    this.isPackaged = true;
  }

  this.categories = aCategories.slice(0);

  this.registryDir = aRegistryDir || OS.Constants.Path.profileDir;

  this._dryRun = false;
  try {
    if (Services.prefs.getBoolPref("browser.mozApps.installer.dry_run")) {
      this._dryRun = true;
    }
  } catch (ex) {}
}

CommonNativeApp.prototype = {
  uniqueName: null,
  appLocalizedName: null,
  appNameAsFilename: null,
  iconURI: null,
  developerName: null,
  shortDescription: null,
  categories: null,
  webappJson: null,
  runtimeFolder: null,
  manifest: null,
  registryDir: null,

  






  _setData: function(aApp, aManifest) {
    let manifest = new ManifestHelper(aManifest, aApp.origin, aApp.manifestURL);
    let origin = Services.io.newURI(aApp.origin, null, null);

    this.iconURI = Services.io.newURI(manifest.biggestIconURL || DEFAULT_ICON_URL,
                                      null, null);

    if (manifest.developer) {
      if (manifest.developer.name) {
        let devName = manifest.developer.name.substr(0, 128);
        if (devName) {
          this.developerName = devName;
        }
      }

      if (manifest.developer.url) {
        this.developerUrl = manifest.developer.url;
      }
    }

    if (manifest.description) {
      let firstLine = manifest.description.split("\n")[0];
      let shortDesc = firstLine.length <= 256
                      ? firstLine
                      : firstLine.substr(0, 253) + "â€¦";
      this.shortDescription = shortDesc;
    } else {
      this.shortDescription = this.appLocalizedName;
    }

    if (manifest.version) {
      this.version = manifest.version;
    }

    this.webappJson = {
      
      
      "registryDir": this.registryDir,
      "app": {
        "manifest": aManifest,
        "origin": aApp.origin,
        "manifestURL": aApp.manifestURL,
        "installOrigin": aApp.installOrigin,
        "categories": this.categories,
        "receipts": aApp.receipts,
        "installTime": aApp.installTime,
      }
    };

    if (aApp.etag) {
      this.webappJson.app.etag = aApp.etag;
    }

    if (aApp.packageEtag) {
      this.webappJson.app.packageEtag = aApp.packageEtag;
    }

    if (aApp.updateManifest) {
      this.webappJson.app.updateManifest = aApp.updateManifest;
    }

    this.runtimeFolder = OS.Constants.Path.libDir;
  },

  



  _getIcon: function(aTmpDir) {
    try {
      
      
      
      if (this.iconURI.scheme == "app") {
        let zipUrl = OS.Path.toFileURI(OS.Path.join(aTmpDir,
                                                    this.zipFile));

        let filePath = this.iconURI.QueryInterface(Ci.nsIURL).filePath;

        this.iconURI = Services.io.newURI("jar:" + zipUrl + "!" + filePath,
                                          null, null);
      }


      let [ mimeType, icon ] = yield downloadIcon(this.iconURI);
      yield this._processIcon(mimeType, icon, aTmpDir);
    }
    catch(e) {
      Cu.reportError("Failure retrieving icon: " + e);

      let iconURI = Services.io.newURI(DEFAULT_ICON_URL, null, null);

      let [ mimeType, icon ] = yield downloadIcon(iconURI);
      yield this._processIcon(mimeType, icon, aTmpDir);

      
      
      this.iconURI = iconURI;
    }
  },

  


  createProfile: function() {
    if (this._dryRun) {
      return null;
    }

    let profSvc = Cc["@mozilla.org/toolkit/profile-service;1"].
                  getService(Ci.nsIToolkitProfileService);

    try {
      let appProfile = profSvc.createDefaultProfileForApp(this.uniqueName,
                                                          null, null);
      return appProfile.localDir;
    } catch (ex if ex.result == Cr.NS_ERROR_ALREADY_INITIALIZED) {
      return null;
    }
  },
};

#ifdef XP_WIN

#include WinNativeApp.js

#elifdef XP_MACOSX

#include MacNativeApp.js

#elifdef XP_UNIX

#include LinuxNativeApp.js

#endif









function writeToFile(aPath, aData) {
  return Task.spawn(function() {
    let data = new TextEncoder().encode(aData);

    let file;
    try {
      file = yield OS.File.open(aPath, { truncate: true, write: true },
                                { unixMode: PERMS_FILE });
      yield file.write(data);
    } finally {
      yield file.close();
    }
  });
}






function stripStringForFilename(aPossiblyBadFilenameString) {
  
  let stripFrontRE = new RegExp("^\\W*", "gi");

  
  let stripBackRE = new RegExp("\\s*$", "gi");

  
  let filenameRE = new RegExp("[<>:\"/\\\\|\\?\\*]", "gi");

  let stripped = aPossiblyBadFilenameString.replace(stripFrontRE, "");
  stripped = stripped.replace(stripBackRE, "");
  stripped = stripped.replace(filenameRE, "");

  
  if (stripped == "") {
    stripped = "webapp";
  }

  return stripped;
}












function getAvailableFileName(aPathSet, aName, aExtension) {
  return Task.spawn(function*() {
    let name = aName + aExtension;

    function checkUnique(aName) {
      return Task.spawn(function*() {
        for (let path of aPathSet) {
          if (yield OS.File.exists(OS.Path.join(path, aName))) {
            return false;
          }
        }

        return true;
      });
    }

    if (yield checkUnique(name)) {
      return name;
    }

    
    
    for (let i = 2; i < 100; i++) {
      name = aName + " (" + i + ")" + aExtension;

      if (yield checkUnique(name)) {
        return name;
      }
    }

    throw "No available filename";
  });
}






function removeFiles(aPaths) {
  for (let path of aPaths) {
    let file = getFile(path);

    try {
      if (file.exists()) {
        file.followLinks = false;
        file.remove(true);
      }
    } catch(ex) {}
  }
}







function moveDirectory(srcPath, destPath) {
  let srcDir = getFile(srcPath);
  let destDir = getFile(destPath);

  let entries = srcDir.directoryEntries;
  let array = [];
  while (entries.hasMoreElements()) {
    let entry = entries.getNext().QueryInterface(Ci.nsIFile);
    if (entry.isDirectory()) {
      yield moveDirectory(entry.path, OS.Path.join(destPath, entry.leafName));
    } else {
      entry.moveTo(destDir, entry.leafName);
    }
  }

  
  yield OS.File.removeEmptyDir(srcPath);
}

function escapeXML(aStr) {
  return aStr.toString()
             .replace(/&/g, "&amp;")
             .replace(/"/g, "&quot;")
             .replace(/'/g, "&apos;")
             .replace(/</g, "&lt;")
             .replace(/>/g, "&gt;");
}


function getFile() {
  let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
  file.initWithPath(OS.Path.join.apply(OS.Path, arguments));
  return file;
}


function downloadIcon(aIconURI) {
  let deferred = Promise.defer();

  let mimeService = Cc["@mozilla.org/mime;1"].getService(Ci.nsIMIMEService);
  let mimeType;
  try {
    let tIndex = aIconURI.path.indexOf(";");
    if("data" == aIconURI.scheme && tIndex != -1) {
      mimeType = aIconURI.path.substring(0, tIndex);
    } else {
      mimeType = mimeService.getTypeFromURI(aIconURI);
     }
  } catch(e) {
    deferred.reject("Failed to determine icon MIME type: " + e);
    return deferred.promise;
  }

  function onIconDownloaded(aStatusCode, aIcon) {
    if (Components.isSuccessCode(aStatusCode)) {
      deferred.resolve([ mimeType, aIcon ]);
    } else {
      deferred.reject("Failure downloading icon: " + aStatusCode);
    }
  }

  try {
#ifdef XP_MACOSX
    let downloadObserver = {
      onDownloadComplete: function(downloader, request, cx, aStatus, file) {
        onIconDownloaded(aStatus, file);
      }
    };

    let tmpIcon = Services.dirsvc.get("TmpD", Ci.nsIFile);
    tmpIcon.append("tmpicon." + mimeService.getPrimaryExtension(mimeType, ""));
    tmpIcon.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, parseInt("666", 8));

    let listener = Cc["@mozilla.org/network/downloader;1"]
                     .createInstance(Ci.nsIDownloader);
    listener.init(downloadObserver, tmpIcon);
#else
    let pipe = Cc["@mozilla.org/pipe;1"]
                 .createInstance(Ci.nsIPipe);
    pipe.init(true, true, 0, 0xffffffff, null);

    let listener = Cc["@mozilla.org/network/simple-stream-listener;1"]
                     .createInstance(Ci.nsISimpleStreamListener);
    listener.init(pipe.outputStream, {
        onStartRequest: function() {},
        onStopRequest: function(aRequest, aContext, aStatusCode) {
          pipe.outputStream.close();
          onIconDownloaded(aStatusCode, pipe.inputStream);
       }
    });
#endif

    
    
    
    
    let principal =
      aIconURI.schemeIs("chrome") ? Services.scriptSecurityManager
                                            .getSystemPrincipal()
                                  : Services.scriptSecurityManager
                                            .getNoAppCodebasePrincipal(aIconURI);

    let channel = NetUtil.newChannel2(aIconURI,
                                      null,
                                      null,
                                      null,      
                                      principal,
                                      null,      
                                      Ci.nsILoadInfo.SEC_NORMAL,
                                      Ci.nsIContentPolicy.TYPE_IMAGE);
    let { BadCertHandler } = Cu.import("resource://gre/modules/CertUtils.jsm", {});
    
    channel.notificationCallbacks = new BadCertHandler(true);

    channel.asyncOpen(listener, null);
  } catch(e) {
    deferred.reject("Failure initiating download of icon: " + e);
  }

  return deferred.promise;
}
