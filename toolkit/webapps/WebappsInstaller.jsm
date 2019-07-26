



this.EXPORTED_SYMBOLS = ["WebappsInstaller"];

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

this.WebappsInstaller = {
  shell: null,

  







  init: function(aData) {
#ifdef XP_WIN
    this.shell = new WinNativeApp(aData);
#elifdef XP_MACOSX
    this.shell = new MacNativeApp(aData);
#elifdef XP_UNIX
    this.shell = new LinuxNativeApp(aData);
#else
    return null;
#endif

    try {
      if (Services.prefs.getBoolPref("browser.mozApps.installer.dry_run")) {
        return this.shell;
      }
    } catch (ex) {}

    try {
      this.shell.createAppProfile();
    } catch (ex) {
      Cu.reportError("Error installing app: " + ex);
      return null;
    }

    return this.shell;
  },

  







  install: function(aData, aManifest, aZipPath) {
    try {
      if (Services.prefs.getBoolPref("browser.mozApps.installer.dry_run")) {
        return Promise.resolve();
      }
    } catch (ex) {}

    this.shell.init(aData, aManifest);

    return this.shell.install(aZipPath).then(() => {
      let data = {
        "installDir": this.shell.installDir.path,
        "app": {
          "manifest": aManifest,
          "origin": aData.app.origin
        }
      };

      Services.obs.notifyObservers(null, "webapp-installed", JSON.stringify(data));
    });
  }
}











function NativeApp(aData) {
  let jsonManifest = aData.isPackage ? aData.app.updateManifest : aData.app.manifest;
  let manifest = new ManifestHelper(jsonManifest, aData.app.origin);

  aData.app.name = manifest.name;
  this.uniqueName = WebappOSUtils.getUniqueName(aData.app);

  this.appName = sanitize(manifest.name);
  this.appNameAsFilename = stripStringForFilename(this.appName);
}

NativeApp.prototype = {
  uniqueName: null,
  appName: null,
  appNameAsFilename: null,
  iconURI: null,
  developerName: null,
  shortDescription: null,
  categories: null,
  webappJson: null,
  runtimeFolder: null,
  manifest: null,

  







  init: function(aData, aManifest) {
    let app = aData.app;
    let manifest = this.manifest = new ManifestHelper(aManifest,
                                                      app.origin);

    let origin = Services.io.newURI(app.origin, null, null);

    let biggestIcon = getBiggestIconURL(manifest.icons);
    try {
      let iconURI = Services.io.newURI(biggestIcon, null, null);
      if (iconURI.scheme == "data") {
        this.iconURI = iconURI;
      }
    } catch (ex) {}

    if (!this.iconURI) {
      try {
        this.iconURI = Services.io.newURI(origin.resolve(biggestIcon), null, null);
      }
      catch (ex) {}
    }

    if (manifest.developer) {
      if (manifest.developer.name) {
        let devName = sanitize(manifest.developer.name.substr(0, 128));
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
      this.shortDescription = sanitize(shortDesc);
    } else {
      this.shortDescription = this.appName;
    }

    this.categories = app.categories.slice(0);

    
    
    let registryFolder = Services.dirsvc.get("ProfD", Ci.nsIFile);

    this.webappJson = {
      "registryDir": registryFolder.path,
      "app": {
        "manifest": aManifest,
        "origin": app.origin,
        "manifestURL": app.manifestURL,
        "installOrigin": app.installOrigin,
        "categories": app.categories,
        "receipts": app.receipts,
        "installTime": app.installTime,
      }
    };

    if (app.etag) {
      this.webappJson.app.etag = app.etag;
    }

    if (app.packageEtag) {
      this.webappJson.app.packageEtag = app.packageEtag;
    }

    if (app.updateManifest) {
      this.webappJson.app.updateManifest = app.updateManifest;
    }

    this.runtimeFolder = Services.dirsvc.get("GreD", Ci.nsIFile);
  },

  



  getIcon: function() {
    try {
      
      
      
      if (this.iconURI.scheme == "app") {
        let zipFile = Cc["@mozilla.org/file/local;1"].
                      createInstance(Ci.nsIFile);
        zipFile.initWithPath(OS.Path.join(this.installDir.path,
                                          "application.zip"));
        let zipUrl = Services.io.newFileURI(zipFile).spec;

        let filePath = this.iconURI.QueryInterface(Ci.nsIURL).filePath;

        this.iconURI = Services.io.newURI("jar:" + zipUrl + "!" + filePath,
                                          null, null);
      }


      let [ mimeType, icon ] = yield downloadIcon(this.iconURI);
      yield this.processIcon(mimeType, icon);
    }
    catch(e) {
      Cu.reportError("Failure retrieving icon: " + e);

      let iconURI = Services.io.newURI(DEFAULT_ICON_URL, null, null);

      let [ mimeType, icon ] = yield downloadIcon(iconURI);
      yield this.processIcon(mimeType, icon);

      
      
      this.iconURI = iconURI;
    }
  },

  


  createAppProfile: function() {
    let profSvc = Cc["@mozilla.org/toolkit/profile-service;1"]
                    .getService(Ci.nsIToolkitProfileService);

    try {
      this.appProfile = profSvc.createDefaultProfileForApp(this.uniqueName,
                                                           null, null);
    } catch (ex if ex.result == Cr.NS_ERROR_ALREADY_INITIALIZED) {}
  },
};

#ifdef XP_WIN




























function WinNativeApp(aData) {
  NativeApp.call(this, aData);

  if (aData.isPackage) {
    this.size = aData.app.updateManifest.size / 1024;
  }

  this._init();
}

WinNativeApp.prototype = {
  __proto__: NativeApp.prototype,
  size: null,

  



  install: function(aZipPath) {
    return Task.spawn(function() {
      try {
        this._copyPrebuiltFiles();
        this._createShortcutFiles();
        this._createConfigFiles();
        this._writeSystemKeys();

        if (aZipPath) {
          yield OS.File.move(aZipPath, OS.Path.join(this.installDir.path,
                                                    "application.zip"));
        }

        yield this.getIcon();
      } catch (ex) {
        this._removeInstallation(false);
        throw(ex);
      }
    }.bind(this));
  },

  



  _init: function() {
    let filenameRE = new RegExp("[<>:\"/\\\\|\\?\\*]", "gi");

    this.appNameAsFilename = this.appNameAsFilename.replace(filenameRE, "");
    if (this.appNameAsFilename == "") {
      this.appNameAsFilename = "webapp";
    }

    
    this.installDir = Services.dirsvc.get("AppData", Ci.nsIFile);
    this.installDir.append(this.uniqueName);

    this.webapprt = this.installDir.clone();
    this.webapprt.append(this.appNameAsFilename + ".exe");

    this.configJson = this.installDir.clone();
    this.configJson.append("webapp.json");

    this.webappINI = this.installDir.clone();
    this.webappINI.append("webapp.ini");

    this.uninstallDir = this.installDir.clone();
    this.uninstallDir.append("uninstall");

    this.uninstallerFile = this.uninstallDir.clone();
    this.uninstallerFile.append("webapp-uninstaller.exe");

    this.iconFile = this.installDir.clone();
    this.iconFile.append("chrome");
    this.iconFile.append("icons");
    this.iconFile.append("default");
    this.iconFile.append("default.ico");

    this.uninstallSubkeyStr = this.uniqueName;

    
    this.shortcutLogsINI = this.uninstallDir.clone();
    this.shortcutLogsINI.append("shortcuts_log.ini");

    if (this.shortcutLogsINI.exists()) {
      
      
      let factory = Cc["@mozilla.org/xpcom/ini-processor-factory;1"]
                      .getService(Ci.nsIINIParserFactory);
      let parser = factory.createINIParser(this.shortcutLogsINI);

      this.shortcutName = parser.getString("STARTMENU", "Shortcut0");
    } else {
      let desktop = Services.dirsvc.get("Desk", Ci.nsIFile);
      let startMenu = Services.dirsvc.get("Progs", Ci.nsIFile);

      
      
      this.shortcutName = getAvailableFileName([ startMenu, desktop ],
                                               this.appNameAsFilename,
                                               ".lnk");
    }

    
    this._removeInstallation(true);

    this._createDirectoryStructure();
  },

  


  _removeInstallation : function(keepProfile) {
    let uninstallKey;
    try {
      uninstallKey = Cc["@mozilla.org/windows-registry-key;1"]
                     .createInstance(Ci.nsIWindowsRegKey);
      uninstallKey.open(uninstallKey.ROOT_KEY_CURRENT_USER,
                        "SOFTWARE\\Microsoft\\Windows\\" +
                        "CurrentVersion\\Uninstall",
                        uninstallKey.ACCESS_WRITE);
      if(uninstallKey.hasChild(this.uninstallSubkeyStr)) {
        uninstallKey.removeChild(this.uninstallSubkeyStr);
      }
    } catch (e) {
    } finally {
      if(uninstallKey)
        uninstallKey.close();
    }

    let desktopShortcut = Services.dirsvc.get("Desk", Ci.nsIFile);
    desktopShortcut.append(this.shortcutName);

    let startMenuShortcut = Services.dirsvc.get("Progs", Ci.nsIFile);
    startMenuShortcut.append(this.shortcutName);

    let filesToRemove = [desktopShortcut, startMenuShortcut];

    if (keepProfile) {
      filesToRemove.push(this.iconFile);
      filesToRemove.push(this.webapprt);
      filesToRemove.push(this.configJson);
      filesToRemove.push(this.webappINI);
      filesToRemove.push(this.uninstallDir);
    } else {
      filesToRemove.push(this.installDir);
    }

    removeFiles(filesToRemove);
  },

  


  _createDirectoryStructure: function() {
    if (!this.installDir.exists()) {
      this.installDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    }

    this.uninstallDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  },

  


  _copyPrebuiltFiles: function() {
    let webapprtPre = this.runtimeFolder.clone();
    webapprtPre.append("webapprt-stub.exe");
    webapprtPre.copyTo(this.installDir, this.webapprt.leafName);

    let uninstaller = this.runtimeFolder.clone();
    uninstaller.append("webapp-uninstaller.exe");
    uninstaller.copyTo(this.uninstallDir, this.uninstallerFile.leafName);
  },

  


  _createConfigFiles: function() {
    
    writeToFile(this.configJson, JSON.stringify(this.webappJson));

    let factory = Cc["@mozilla.org/xpcom/ini-processor-factory;1"]
                    .getService(Ci.nsIINIParserFactory);

    
    let writer = factory.createINIParser(this.webappINI).QueryInterface(Ci.nsIINIParserWriter);
    writer.setString("Webapp", "Name", this.appName);
    writer.setString("Webapp", "Profile", this.installDir.leafName);
    writer.setString("Webapp", "Executable", this.appNameAsFilename);
    writer.setString("WebappRT", "InstallDir", this.runtimeFolder.path);
    writer.writeFile(null, Ci.nsIINIParserWriter.WRITE_UTF16);

    writer = factory.createINIParser(this.shortcutLogsINI).QueryInterface(Ci.nsIINIParserWriter);
    writer.setString("STARTMENU", "Shortcut0", this.shortcutName);
    writer.setString("DESKTOP", "Shortcut0", this.shortcutName);
    writer.setString("TASKBAR", "Migrated", "true");
    writer.writeFile(null, Ci.nsIINIParserWriter.WRITE_UTF16);

    
    let uninstallContent = 
      "File: \\webapp.ini\r\n" +
      "File: \\webapp.json\r\n" +
      "File: \\webapprt.old\r\n" +
      "File: \\chrome\\icons\\default\\default.ico";
    let uninstallLog = this.uninstallDir.clone();
    uninstallLog.append("uninstall.log");
    writeToFile(uninstallLog, uninstallContent);
  },

  



  _writeSystemKeys: function() {
    let parentKey;
    let uninstallKey;
    let subKey;

    try {
      parentKey = Cc["@mozilla.org/windows-registry-key;1"]
                  .createInstance(Ci.nsIWindowsRegKey);
      parentKey.open(parentKey.ROOT_KEY_CURRENT_USER,
                     "SOFTWARE\\Microsoft\\Windows\\CurrentVersion",
                     parentKey.ACCESS_WRITE);
      uninstallKey = parentKey.createChild("Uninstall", parentKey.ACCESS_WRITE)
      subKey = uninstallKey.createChild(this.uninstallSubkeyStr, uninstallKey.ACCESS_WRITE);

      subKey.writeStringValue("DisplayName", this.appName);

      subKey.writeStringValue("UninstallString", '"' + this.uninstallerFile.path + '"');
      subKey.writeStringValue("InstallLocation", '"' + this.installDir.path + '"');
      subKey.writeStringValue("AppFilename", this.appNameAsFilename);

      if(this.iconFile) {
        subKey.writeStringValue("DisplayIcon", this.iconFile.path);
      }

      let date = new Date();
      let year = date.getYear().toString();
      let month = date.getMonth();
      if (month < 10) {
        month = "0" + month;
      }
      let day = date.getDate();
      if (day < 10) {
        day = "0" + day;
      }
      subKey.writeStringValue("InstallDate", year + month + day);
      if (this.manifest.version) {
        subKey.writeStringValue("DisplayVersion", this.manifest.version);
      }
      if (this.developerName) {
        subKey.writeStringValue("Publisher", this.developerName);
      }
      subKey.writeStringValue("URLInfoAbout", this.developerUrl);
      if (this.size) {
        subKey.writeIntValue("EstimatedSize", this.size);
      }

      subKey.writeIntValue("NoModify", 1);
      subKey.writeIntValue("NoRepair", 1);
    } catch(ex) {
      throw(ex);
    } finally {
      if(subKey) subKey.close();
      if(uninstallKey) uninstallKey.close();
      if(parentKey) parentKey.close();
    }
  },

  



  _createShortcutFiles: function() {
    let shortcut = this.installDir.clone().QueryInterface(Ci.nsILocalFileWin);
    shortcut.append(this.shortcutName);

    let target = this.installDir.clone();
    target.append(this.webapprt.leafName);

    


    shortcut.setShortcut(target, this.installDir.clone(), null,
                         this.shortDescription, this.iconFile, 0);

    let desktop = Services.dirsvc.get("Desk", Ci.nsILocalFile);
    let startMenu = Services.dirsvc.get("Progs", Ci.nsILocalFile);

    shortcut.copyTo(desktop, this.shortcutName);
    shortcut.copyTo(startMenu, this.shortcutName);

    shortcut.followLinks = false;
    shortcut.remove(false);
  },

  







  processIcon: function(aMimeType, aImageStream) {
    let deferred = Promise.defer();

    let imgTools = Cc["@mozilla.org/image/tools;1"]
                     .createInstance(Ci.imgITools);

    let imgContainer = imgTools.decodeImage(aImageStream, aMimeType);
    let iconStream = imgTools.encodeImage(imgContainer,
                                          "image/vnd.microsoft.icon",
                                          "format=bmp;bpp=32");

    if (!this.iconFile.parent.exists()) {
      this.iconFile.parent.create(Ci.nsIFile.DIRECTORY_TYPE, parseInt("0755", 8));
    }
    let outputStream = FileUtils.openSafeFileOutputStream(this.iconFile);
    NetUtil.asyncCopy(iconStream, outputStream, function(aResult) {
      if (Components.isSuccessCode(aResult)) {
        deferred.resolve();
      } else {
        deferred.reject("Failure copying icon: " + aResult);
      }
    });

    return deferred.promise;
  }
}

#elifdef XP_MACOSX

function MacNativeApp(aData) {
  NativeApp.call(this, aData);
  this._init();
}

MacNativeApp.prototype = {
  __proto__: NativeApp.prototype,

  _init: function() {
    this.appSupportDir = Services.dirsvc.get("ULibDir", Ci.nsILocalFile);
    this.appSupportDir.append("Application Support");

    let filenameRE = new RegExp("[<>:\"/\\\\|\\?\\*]", "gi");
    this.appNameAsFilename = this.appNameAsFilename.replace(filenameRE, "");
    if (this.appNameAsFilename == "") {
      this.appNameAsFilename = "Webapp";
    }

    
    this.appProfileDir = this.appSupportDir.clone();
    this.appProfileDir.append(this.uniqueName);

    this.installDir = Services.dirsvc.get("TmpD", Ci.nsILocalFile);
    this.installDir.append(this.appNameAsFilename + ".app");
    this.installDir.createUnique(Ci.nsIFile.DIRECTORY_TYPE, 0755);

    this.contentsDir = this.installDir.clone();
    this.contentsDir.append("Contents");

    this.macOSDir = this.contentsDir.clone();
    this.macOSDir.append("MacOS");

    this.resourcesDir = this.contentsDir.clone();
    this.resourcesDir.append("Resources");

    this.iconFile = this.resourcesDir.clone();
    this.iconFile.append("appicon.icns");

    
    this._removeInstallation(true);

    this._createDirectoryStructure();
  },

  install: function(aZipPath) {
    return Task.spawn(function() {
      try {
        this._copyPrebuiltFiles();
        this._createConfigFiles();

        if (aZipPath) {
          yield OS.File.move(aZipPath, OS.Path.join(this.installDir.path,
                                                    "application.zip"));
        }

        yield this.getIcon();
        this._moveToApplicationsFolder();
      } catch (ex) {
        this._removeInstallation(false);
        throw(ex);
      }
    }.bind(this));
  },

  _removeInstallation: function(keepProfile) {
    let filesToRemove = [this.installDir];

    if (!keepProfile) {
      filesToRemove.push(this.appProfileDir);
    }

    removeFiles(filesToRemove);
  },

  _createDirectoryStructure: function() {
    if (!this.appProfileDir.exists()) {
      this.appProfileDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    }

    this.contentsDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    this.macOSDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    this.resourcesDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  },

  _copyPrebuiltFiles: function() {
    let webapprt = this.runtimeFolder.clone();
    webapprt.append("webapprt-stub");
    webapprt.copyTo(this.macOSDir, "webapprt");
  },

  _createConfigFiles: function() {
    
    let configJson = this.appProfileDir.clone();
    configJson.append("webapp.json");
    writeToFile(configJson, JSON.stringify(this.webappJson));

    
    let applicationINI = this.macOSDir.clone().QueryInterface(Ci.nsILocalFile);
    applicationINI.append("webapp.ini");

    let factory = Cc["@mozilla.org/xpcom/ini-processor-factory;1"]
                    .getService(Ci.nsIINIParserFactory);

    let writer = factory.createINIParser(applicationINI).QueryInterface(Ci.nsIINIParserWriter);
    writer.setString("Webapp", "Name", this.appName);
    writer.setString("Webapp", "Profile", this.appProfileDir.leafName);
    writer.writeFile();
    applicationINI.permissions = FileUtils.PERMS_FILE;

    
    let infoPListContent = '<?xml version="1.0" encoding="UTF-8"?>\n\
<!DOCTYPE plist PUBLIC "-//Apple//DTD PLIST 1.0//EN" "http://www.apple.com/DTDs/PropertyList-1.0.dtd">\n\
<plist version="1.0">\n\
  <dict>\n\
    <key>CFBundleDevelopmentRegion</key>\n\
    <string>English</string>\n\
    <key>CFBundleDisplayName</key>\n\
    <string>' + escapeXML(this.appName) + '</string>\n\
    <key>CFBundleExecutable</key>\n\
    <string>webapprt</string>\n\
    <key>CFBundleIconFile</key>\n\
    <string>appicon</string>\n\
    <key>CFBundleIdentifier</key>\n\
    <string>' + escapeXML(this.uniqueName) + '</string>\n\
    <key>CFBundleInfoDictionaryVersion</key>\n\
    <string>6.0</string>\n\
    <key>CFBundleName</key>\n\
    <string>' + escapeXML(this.appName) + '</string>\n\
    <key>CFBundlePackageType</key>\n\
    <string>APPL</string>\n\
    <key>CFBundleVersion</key>\n\
    <string>0</string>\n\
    <key>NSHighResolutionCapable</key>\n\
    <true/>\n\
    <key>NSPrincipalClass</key>\n\
    <string>GeckoNSApplication</string>\n\
    <key>FirefoxBinary</key>\n\
#expand     <string>__MOZ_MACBUNDLE_ID__</string>\n\
  </dict>\n\
</plist>';

    let infoPListFile = this.contentsDir.clone();
    infoPListFile.append("Info.plist");
    writeToFile(infoPListFile, infoPListContent);
  },

  _moveToApplicationsFolder: function() {
    let appDir = Services.dirsvc.get("LocApp", Ci.nsILocalFile);
    let destinationName = getAvailableFileName([appDir],
                                               this.appNameAsFilename,
                                              ".app");
    if (!destinationName) {
      throw("No available filename");
    }
    this.installDir.moveTo(appDir, destinationName);
  },

  







  processIcon: function(aMimeType, aIcon) {
    let deferred = Promise.defer();

    function conversionDone(aSubject, aTopic) {
      if (aTopic == "process-finished") {
        deferred.resolve();
      } else {
        deferred.reject("Failure converting icon.");
      }
    }

    let process = Cc["@mozilla.org/process/util;1"].
                  createInstance(Ci.nsIProcess);
    let sipsFile = Cc["@mozilla.org/file/local;1"].
                   createInstance(Ci.nsILocalFile);
    sipsFile.initWithPath("/usr/bin/sips");

    process.init(sipsFile);
    process.runAsync(["-s",
                "format", "icns",
                aIcon.path,
                "--out", this.iconFile.path,
                "-z", "128", "128"],
                9, conversionDone);

    return deferred.promise;
  }

}

#elifdef XP_UNIX

function LinuxNativeApp(aData) {
  NativeApp.call(this, aData);
  this._init();
}

LinuxNativeApp.prototype = {
  __proto__: NativeApp.prototype,

  _init: function() {
    
    

    this.installDir = Services.dirsvc.get("Home", Ci.nsIFile);
    this.installDir.append("." + this.uniqueName);

    this.iconFile = this.installDir.clone();
    this.iconFile.append("icon.png");

    this.webapprt = this.installDir.clone();
    this.webapprt.append("webapprt-stub");

    this.configJson = this.installDir.clone();
    this.configJson.append("webapp.json");

    this.webappINI = this.installDir.clone();
    this.webappINI.append("webapp.ini");

    let env = Cc["@mozilla.org/process/environment;1"]
                .getService(Ci.nsIEnvironment);
    let xdg_data_home_env = env.get("XDG_DATA_HOME");
    if (xdg_data_home_env != "") {
      this.desktopINI = Cc["@mozilla.org/file/local;1"]
                          .createInstance(Ci.nsILocalFile);
      this.desktopINI.initWithPath(xdg_data_home_env);
    }
    else {
      this.desktopINI = Services.dirsvc.get("Home", Ci.nsIFile);
      this.desktopINI.append(".local");
      this.desktopINI.append("share");
    }

    this.desktopINI.append("applications");
    this.desktopINI.append("owa-" + this.uniqueName + ".desktop");

    
    this._removeInstallation(true);

    this._createDirectoryStructure();
  },

  install: function(aZipPath) {
    return Task.spawn(function() {
      try {
        this._copyPrebuiltFiles();
        this._createConfigFiles();

        if (aZipPath) {
          yield OS.File.move(aZipPath, OS.Path.join(this.installDir.path,
                                                    "application.zip"));
        }

        yield this.getIcon();
      } catch (ex) {
        this._removeInstallation(false);
        throw(ex);
      }
    }.bind(this));
  },

  _removeInstallation: function(keepProfile) {
    let filesToRemove = [this.desktopINI];

    if (keepProfile) {
      filesToRemove.push(this.iconFile);
      filesToRemove.push(this.webapprt);
      filesToRemove.push(this.configJson);
      filesToRemove.push(this.webappINI);
    } else {
      filesToRemove.push(this.installDir);
    }

    removeFiles(filesToRemove);
  },

  _createDirectoryStructure: function() {
    if (!this.installDir.exists())
      this.installDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  },

  _copyPrebuiltFiles: function() {
    let webapprtPre = this.runtimeFolder.clone();
    webapprtPre.append(this.webapprt.leafName);
    webapprtPre.copyTo(this.installDir, this.webapprt.leafName);
  },

  






  _translateCategories: function() {
    let translations = {
      "books": "Education;Literature",
      "business": "Finance",
      "education": "Education",
      "entertainment": "Amusement",
      "sports": "Sports",
      "games": "Game",
      "health-fitness": "MedicalSoftware",
      "lifestyle": "Amusement",
      "music": "Audio;Music",
      "news-weather": "News",
      "photo-video": "Video;AudioVideo;Photography",
      "productivity": "Office",
      "shopping": "Amusement",
      "social": "Chat",
      "travel": "Amusement",
      "reference": "Science;Education;Documentation",
      "maps-navigation": "Maps",
      "utilities": "Utility"
    };

    
    let categories = "";
    for (let category of this.categories) {
      let catLower = category.toLowerCase();
      if (catLower in translations) {
        categories += translations[catLower] + ";";
      }
    }

    return categories;
  },

  _createConfigFiles: function() {
    
    writeToFile(this.configJson, JSON.stringify(this.webappJson));

    let factory = Cc["@mozilla.org/xpcom/ini-processor-factory;1"]
                    .getService(Ci.nsIINIParserFactory);

    let webappsBundle = Services.strings.createBundle("chrome://global/locale/webapps.properties");

    
    let writer = factory.createINIParser(this.webappINI).QueryInterface(Ci.nsIINIParserWriter);
    writer.setString("Webapp", "Name", this.appName);
    writer.setString("Webapp", "Profile", this.uniqueName);
    writer.setString("Webapp", "UninstallMsg", webappsBundle.formatStringFromName("uninstall.notification", [this.appName], 1));
    writer.setString("WebappRT", "InstallDir", this.runtimeFolder.path);
    writer.writeFile();

    
    this.desktopINI.create(Ci.nsIFile.NORMAL_FILE_TYPE, 0755);

    writer = factory.createINIParser(this.desktopINI).QueryInterface(Ci.nsIINIParserWriter);
    writer.setString("Desktop Entry", "Name", this.appName);
    writer.setString("Desktop Entry", "Comment", this.shortDescription);
    writer.setString("Desktop Entry", "Exec", '"'+this.webapprt.path+'"');
    writer.setString("Desktop Entry", "Icon", this.iconFile.path);
    writer.setString("Desktop Entry", "Type", "Application");
    writer.setString("Desktop Entry", "Terminal", "false");

    let categories = this._translateCategories();
    if (categories)
      writer.setString("Desktop Entry", "Categories", categories);

    writer.setString("Desktop Entry", "Actions", "Uninstall;");
    writer.setString("Desktop Action Uninstall", "Name", webappsBundle.GetStringFromName("uninstall.label"));
    writer.setString("Desktop Action Uninstall", "Exec", this.webapprt.path + " -remove");

    writer.writeFile();
  },

  






  processIcon: function(aMimeType, aImageStream) {
    let deferred = Promise.defer();

    let imgTools = Cc["@mozilla.org/image/tools;1"]
                     .createInstance(Ci.imgITools);

    let imgContainer = imgTools.decodeImage(aImageStream, aMimeType);
    let iconStream = imgTools.encodeImage(imgContainer, "image/png");

    let outputStream = FileUtils.openSafeFileOutputStream(this.iconFile);
    NetUtil.asyncCopy(iconStream, outputStream, function(aResult) {
      if (Components.isSuccessCode(aResult)) {
        deferred.resolve();
      } else {
        deferred.reject("Failure copying icon: " + aResult);
      }
    });

    return deferred.promise;
  }
}

#endif









function writeToFile(aFile, aData) {
  return Task.spawn(function() {
    let data = new TextEncoder().encode(aData);
    let file = yield OS.File.open(aFile.path, { truncate: true }, { unixMode: FileUtils.PERMS_FILE });
    yield file.write(data);
    yield file.close();
  });
}




function sanitize(aStr) {
  let unprintableRE = new RegExp("[\\x00-\\x1F\\x7F]" ,"gi");
  return aStr.replace(unprintableRE, "");
}




function stripStringForFilename(aPossiblyBadFilenameString) {
  

  let stripFrontRE = new RegExp("^\\W*","gi");
  let stripBackRE = new RegExp("\\s*$","gi");

  let stripped = aPossiblyBadFilenameString.replace(stripFrontRE, "");
  stripped = stripped.replace(stripBackRE, "");
  return stripped;
}












function getAvailableFileName(aFolderSet, aName, aExtension) {
  let fileSet = [];
  let name = aName + aExtension;
  let isUnique = true;

  
  for (let folder of aFolderSet) {
    folder.followLinks = false;
    if (!folder.isDirectory() || !folder.isWritable()) {
      return null;
    }

    let file = folder.clone();
    file.append(name);
    
    
    if (isUnique && file.exists()) {
      isUnique = false;
    }

    fileSet.push(file);
  }

  if (isUnique) {
    return name;
  }


  function checkUnique(aName) {
    for (let file of fileSet) {
      file.leafName = aName;

      if (file.exists()) {
        return false;
      }
    }

    return true;
  }

  
  
  for (let i = 2; i < 100; i++) {
    name = aName + " (" + i + ")" + aExtension;

    if (checkUnique(name)) {
      return name;
    }
  }

  return null;
}






function removeFiles(aFiles) {
  for (let file of aFiles) {
    try {
      if (file.exists()) {
        file.followLinks = false;
        file.remove(true);
      }
    } catch(ex) {}
  }
}

function escapeXML(aStr) {
  return aStr.toString()
             .replace(/&/g, "&amp;")
             .replace(/"/g, "&quot;")
             .replace(/'/g, "&apos;")
             .replace(/</g, "&lt;")
             .replace(/>/g, "&gt;");
}


#include WebappsIconHelpers.js
