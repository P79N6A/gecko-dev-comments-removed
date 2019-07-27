



const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu, Constructor: CC } = Components;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/Promise.jsm");

#ifndef MOZ_B2G
#ifdef XP_MACOSX
Cu.import("resource://gre/modules/osfile.jsm");
#endif
#endif

this.EXPORTED_SYMBOLS = ["WebappOSUtils"];


function computeHash(aString) {
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                  createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  let result = {};
  
  let data = converter.convertToByteArray(aString, result);

  let hasher = Cc["@mozilla.org/security/hash;1"].
               createInstance(Ci.nsICryptoHash);
  hasher.init(hasher.MD5);
  hasher.update(data, data.length);
  
  let hash = hasher.finish(false);

  function toHexString(charCode) {
    return ("0" + charCode.toString(16)).slice(-2);
  }

  
  return [toHexString(hash.charCodeAt(i)) for (i in hash)].join("");
}

this.WebappOSUtils = {
  getUniqueName: function(aApp) {
    return this.sanitizeStringForFilename(aApp.name).toLowerCase() + "-" +
           computeHash(aApp.manifestURL);
  },

#ifdef XP_WIN
  



  getAppRegKey: function(aApp) {
    let regKey = Cc["@mozilla.org/windows-registry-key;1"].
                 createInstance(Ci.nsIWindowsRegKey);

    try {
      regKey.open(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                  "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" +
                  this.getUniqueName(aApp), Ci.nsIWindowsRegKey.ACCESS_READ);

      return { value: regKey,
               namingSchemeVersion: 2};
    } catch (ex) {}

    
    try {
      regKey.open(Ci.nsIWindowsRegKey.ROOT_KEY_CURRENT_USER,
                  "SOFTWARE\\Microsoft\\Windows\\CurrentVersion\\Uninstall\\" +
                  aApp.origin, Ci.nsIWindowsRegKey.ACCESS_READ);

      return { value: regKey,
               namingSchemeVersion: 1 };
    } catch (ex) {}

    return null;
  },
#endif

  








  getLaunchTarget: function(aApp) {
#ifdef XP_WIN
    let appRegKey = this.getAppRegKey(aApp);

    if (!appRegKey) {
      return null;
    }

    let appFilename, installLocation;
    try {
      appFilename = appRegKey.value.readStringValue("AppFilename");
      installLocation = appRegKey.value.readStringValue("InstallLocation");
    } catch (ex) {
      return null;
    } finally {
      appRegKey.value.close();
    }

    installLocation = installLocation.substring(1, installLocation.length - 1);

    if (appRegKey.namingSchemeVersion == 1 &&
        !this.isOldInstallPathValid(aApp, installLocation)) {
      return null;
    }

    let initWithPath = CC("@mozilla.org/file/local;1",
                          "nsILocalFile", "initWithPath");
    let launchTarget = initWithPath(installLocation);
    launchTarget.append(appFilename + ".exe");

    return launchTarget;
#elifdef XP_MACOSX
    let uniqueName = this.getUniqueName(aApp);

    let mwaUtils = Cc["@mozilla.org/widget/mac-web-app-utils;1"].
                   createInstance(Ci.nsIMacWebAppUtils);

    try {
      let path;
      if (path = mwaUtils.pathForAppWithIdentifier(uniqueName)) {
        return [ uniqueName, path ];
      }
    } catch(ex) {}

    
    try {
      let path;
      if ((path = mwaUtils.pathForAppWithIdentifier(aApp.origin)) &&
           this.isOldInstallPathValid(aApp, path)) {
        return [ aApp.origin, path ];
      }
    } catch(ex) {}

    return [ null, null ];
#elifdef XP_UNIX
    let uniqueName = this.getUniqueName(aApp);

    let exeFile = Services.dirsvc.get("Home", Ci.nsIFile);
    exeFile.append("." + uniqueName);
    exeFile.append("webapprt-stub");

    
    if (!exeFile.exists()) {
      exeFile = Services.dirsvc.get("Home", Ci.nsIFile);

      let origin = Services.io.newURI(aApp.origin, null, null);
      let installDir = "." + origin.scheme + ";" +
                       origin.host +
                       (origin.port != -1 ? ";" + origin.port : "");

      exeFile.append(installDir);
      exeFile.append("webapprt-stub");

      if (!exeFile.exists() ||
          !this.isOldInstallPathValid(aApp, exeFile.parent.path)) {
        return null;
      }
    }

    return exeFile;
#endif
  },

  getInstallPath: function(aApp) {
#ifdef MOZ_B2G
    
    return aApp.basePath + "/" + aApp.id;

#elifdef MOZ_FENNEC
   
    return aApp.basePath + "/" + aApp.id;

#elifdef MOZ_PHOENIX
   

#ifdef XP_WIN
    let execFile = this.getLaunchTarget(aApp);
    if (!execFile) {
      return null;
    }

    return execFile.parent.path;
#elifdef XP_MACOSX
    let [ bundleID, path ] = this.getLaunchTarget(aApp);
    return path;
#elifdef XP_UNIX
    let execFile = this.getLaunchTarget(aApp);
    if (!execFile) {
      return null;
    }

    return execFile.parent.path;
#endif

#elifdef MOZ_WEBAPP_RUNTIME
    

#ifdef XP_WIN
    let execFile = this.getLaunchTarget(aApp);
    if (!execFile) {
      return null;
    }

    return execFile.parent.path;
#elifdef XP_MACOSX
    let [ bundleID, path ] = this.getLaunchTarget(aApp);
    return path;
#elifdef XP_UNIX
    let execFile = this.getLaunchTarget(aApp);
    if (!execFile) {
      return null;
    }

    return execFile.parent.path;
#endif

#endif
    
    throw new Error("Unsupported apps platform");
  },

  getPackagePath: function(aApp) {
    let packagePath = this.getInstallPath(aApp);

    
#ifndef MOZ_B2G
#ifdef XP_MACOSX
    packagePath = OS.Path.join(packagePath, "Contents", "Resources");
#endif
#endif

    return packagePath;
  },

  launch: function(aApp) {
    let uniqueName = this.getUniqueName(aApp);

#ifdef XP_WIN
    let launchTarget = this.getLaunchTarget(aApp);
    if (!launchTarget) {
      return false;
    }

    try {
      let process = Cc["@mozilla.org/process/util;1"].
                    createInstance(Ci.nsIProcess);

      process.init(launchTarget);
      process.runwAsync([], 0);
    } catch (e) {
      return false;
    }

    return true;
#elifdef XP_MACOSX
    let [ launchIdentifier, path ] = this.getLaunchTarget(aApp);
    if (!launchIdentifier) {
      return false;
    }

    let mwaUtils = Cc["@mozilla.org/widget/mac-web-app-utils;1"].
                   createInstance(Ci.nsIMacWebAppUtils);

    try {
      mwaUtils.launchAppWithIdentifier(launchIdentifier);
    } catch(e) {
      return false;
    }

    return true;
#elifdef XP_UNIX
    let exeFile = this.getLaunchTarget(aApp);
    if (!exeFile) {
      return false;
    }

    try {
      let process = Cc["@mozilla.org/process/util;1"]
                      .createInstance(Ci.nsIProcess);

      process.init(exeFile);
      process.runAsync([], 0);
    } catch (e) {
      return false;
    }

    return true;
#endif
  },

  uninstall: function(aApp) {
#ifdef XP_WIN
    let appRegKey = this.getAppRegKey(aApp);

    if (!appRegKey) {
      return Promise.reject("App registry key not found");
    }

    let deferred = Promise.defer();

    try {
      let uninstallerPath = appRegKey.value.readStringValue("UninstallString");
      uninstallerPath = uninstallerPath.substring(1, uninstallerPath.length - 1);

      let uninstaller = Cc["@mozilla.org/file/local;1"].
                        createInstance(Ci.nsIFile);
      uninstaller.initWithPath(uninstallerPath);

      let process = Cc["@mozilla.org/process/util;1"].
                    createInstance(Ci.nsIProcess);
      process.init(uninstaller);
      process.runwAsync(["/S"], 1, (aSubject, aTopic) => {
        if (aTopic == "process-finished") {
          deferred.resolve(true);
        } else {
          deferred.reject("Uninstaller failed with exit code: " + aSubject.exitValue);
        }
      });
    } catch (e) {
      deferred.reject(e);
    } finally {
      appRegKey.value.close();
    }

    return deferred.promise;
#elifdef XP_MACOSX
    let [ , path ] = this.getLaunchTarget(aApp);
    if (!path) {
      return Promise.reject("App not found");
    }

    let deferred = Promise.defer();

    let mwaUtils = Cc["@mozilla.org/widget/mac-web-app-utils;1"].
                   createInstance(Ci.nsIMacWebAppUtils);

    mwaUtils.trashApp(path, (aResult) => {
      if (aResult == Cr.NS_OK) {
        deferred.resolve(true);
      } else {
        deferred.reject("Error moving the app to the Trash: " + aResult);
      }
    });

    return deferred.promise;
#elifdef XP_UNIX
    let exeFile = this.getLaunchTarget(aApp);
    if (!exeFile) {
      return Promise.reject("App executable file not found");
    }

    let deferred = Promise.defer();

    try {
      let process = Cc["@mozilla.org/process/util;1"]
                      .createInstance(Ci.nsIProcess);

      process.init(exeFile);
      process.runAsync(["-remove"], 1, (aSubject, aTopic) => {
        if (aTopic == "process-finished") {
          deferred.resolve(true);
        } else {
          deferred.reject("Uninstaller failed with exit code: " + aSubject.exitValue);
        }
      });
    } catch (e) {
      deferred.reject(e);
    }

    return deferred.promise;
#endif
  },

  



  isOldInstallPathValid: function(aApp, aInstallPath) {
    
    
    
    
    if (aApp.origin.startsWith("app")) {
      return false;
    }

    
    
    return true;
  },

  


  isLaunchable: function(aApp) {
#ifdef MOZ_WIDGET_ANDROID
    return true;
#endif

    let uniqueName = this.getUniqueName(aApp);

#ifdef XP_WIN
    if (!this.getLaunchTarget(aApp)) {
      return false;
    }

    return true;
#elifdef XP_MACOSX
    if (!this.getInstallPath(aApp)) {
      return false;
    }

    return true;
#elifdef XP_UNIX
    let env = Cc["@mozilla.org/process/environment;1"]
                .getService(Ci.nsIEnvironment);

    let xdg_data_home_env;
    try {
      xdg_data_home_env = env.get("XDG_DATA_HOME");
    } catch(ex) {}

    let desktopINI;
    if (xdg_data_home_env) {
      desktopINI = new FileUtils.File(xdg_data_home_env);
    } else {
      desktopINI = FileUtils.getFile("Home", [".local", "share"]);
    }
    desktopINI.append("applications");
    desktopINI.append("owa-" + uniqueName + ".desktop");

    
    if (!desktopINI.exists()) {
      if (xdg_data_home_env) {
        desktopINI = new FileUtils.File(xdg_data_home_env);
      } else {
        desktopINI = FileUtils.getFile("Home", [".local", "share"]);
      }

      let origin = Services.io.newURI(aApp.origin, null, null);
      let oldUniqueName = origin.scheme + ";" +
                          origin.host +
                          (origin.port != -1 ? ";" + origin.port : "");

      desktopINI.append("owa-" + oldUniqueName + ".desktop");

      if (!desktopINI.exists()) {
        return false;
      }

      let installDir = Services.dirsvc.get("Home", Ci.nsIFile);
      installDir.append("." + origin.scheme + ";" + origin.host +
                        (origin.port != -1 ? ";" + origin.port : ""));

      return isOldInstallPathValid(aApp, installDir.path);
    }

    return true;
#endif
  },

  


  sanitizeStringForFilename: function(aPossiblyBadFilenameString) {
    return aPossiblyBadFilenameString.replace(/[^a-z0-9_\-]/gi, "");
  }
}
