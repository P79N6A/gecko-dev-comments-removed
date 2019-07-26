



this.EXPORTED_SYMBOLS = ["WebappsInstaller"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;
const Cr = Components.results;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");
Cu.import("resource://gre/modules/osfile.jsm");

this.WebappsInstaller = {
  






  install: function(aData) {

    try {
      if (Services.prefs.getBoolPref("browser.mozApps.installer.dry_run")) {
        return true;
      }
    } catch (ex) {}

#ifdef XP_WIN
    let shell = new WinNativeApp(aData);
#elifdef XP_MACOSX
    let shell = new MacNativeApp(aData);
#elifdef XP_UNIX
    let shell = new LinuxNativeApp(aData);
#else
    return false;
#endif

    try {
      shell.install();
    } catch (ex) {
      Cu.reportError("Error installing app: " + ex);
      return null;
    }

    let data = {
      "installDir": shell.installDir.path,
      "app": aData.app
    };
    Services.obs.notifyObservers(null, "webapp-installed", JSON.stringify(data));

    return shell;
  }
}












function NativeApp(aData) {
  let app = this.app = aData.app;

  let origin = Services.io.newURI(app.origin, null, null);

  if (app.manifest.launch_path) {
    this.launchURI = Services.io.newURI(origin.resolve(app.manifest.launch_path),
                                        null, null);
  } else {
    this.launchURI = origin.clone();
  }

  let biggestIcon = getBiggestIconURL(app.manifest.icons);
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

  this.appName = sanitize(app.manifest.name);
  this.appNameAsFilename = stripStringForFilename(this.appName);

  if(app.manifest.developer && app.manifest.developer.name) {
    let devName = app.manifest.developer.name.substr(0, 128);
    devName = sanitize(devName);
    if (devName) {
      this.developerName = devName;
    }
  }

  let shortDesc = this.appName;
  if (app.manifest.description) {
    let firstLine = app.manifest.description.split("\n")[0];
    shortDesc = firstLine.length <= 256
                ? firstLine
                : firstLine.substr(0, 253) + "...";
  }
  this.shortDescription = sanitize(shortDesc);

  
  
  this.registryFolder = Services.dirsvc.get("ProfD", Ci.nsIFile);

  this.webappJson = {
    "registryDir": this.registryFolder.path,
    "app": app
  };

  this.runtimeFolder = Services.dirsvc.get("GreD", Ci.nsIFile);
}

#ifdef XP_WIN






























function WinNativeApp(aData) {
  NativeApp.call(this, aData);
  this._init();
}

WinNativeApp.prototype = {
  



  install: function() {
    
    this._removeInstallation(true);

    try {
      this._createDirectoryStructure();
      this._copyPrebuiltFiles();
      this._createConfigFiles();
      this._createShortcutFiles();
      this._writeSystemKeys();
      this._createAppProfile();
    } catch (ex) {
      this._removeInstallation(false);
      throw(ex);
    }

    getIconForApp(this, function() {});
  },

  



  _init: function() {
    let filenameRE = new RegExp("[<>:\"/\\\\|\\?\\*]", "gi");

    this.appNameAsFilename = this.appNameAsFilename.replace(filenameRE, "");
    if (this.appNameAsFilename == "") {
      this.appNameAsFilename = "webapp";
    }

    
    
    
    
    this.installDir = Services.dirsvc.get("AppData", Ci.nsIFile);
    let installDirLeaf = this.launchURI.scheme
                       + ";"
                       + this.launchURI.host;
    if (this.launchURI.port != -1) {
      installDirLeaf += ";" + this.launchURI.port;
    }
    this.installDir.append(installDirLeaf);

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

    this.uninstallSubkeyStr = this.launchURI.scheme + "://" +
                              this.launchURI.hostPort;
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

    let desktopShortcut = Services.dirsvc.get("Desk", Ci.nsILocalFile);
    desktopShortcut.append(this.appNameAsFilename + ".lnk");

    let startMenuShortcut = Services.dirsvc.get("Progs", Ci.nsILocalFile);
    startMenuShortcut.append(this.appNameAsFilename + ".lnk");

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
    if (!this.installDir.exists())
      this.installDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    this.uninstallDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  },

  


  _createAppProfile: function() {
    let profSvc = Cc["@mozilla.org/toolkit/profile-service;1"]
                    .getService(Ci.nsIToolkitProfileService);

    try {
      this.appProfile = profSvc.createDefaultProfileForApp(this.installDir.leafName,
                                                           null, null);
    } catch (ex if ex.result == Cr.NS_ERROR_ALREADY_INITIALIZED) {}
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

    
    let shortcutLogsINI = this.uninstallDir.clone().QueryInterface(Ci.nsILocalFile);
    shortcutLogsINI.append("shortcuts_log.ini");

    writer = factory.createINIParser(shortcutLogsINI).QueryInterface(Ci.nsIINIParserWriter);
    writer.setString("STARTMENU", "Shortcut0", this.appNameAsFilename + ".lnk");
    writer.setString("DESKTOP", "Shortcut0", this.appNameAsFilename + ".lnk");
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

      subKey.writeStringValue("UninstallString", this.uninstallerFile.path);
      subKey.writeStringValue("InstallLocation", this.installDir.path);
      subKey.writeStringValue("AppFilename", this.appNameAsFilename);

      if(this.iconFile) {
        subKey.writeStringValue("DisplayIcon", this.iconFile.path);
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
    shortcut.append(this.appNameAsFilename + ".lnk");

    let target = this.installDir.clone();
    target.append(this.webapprt.leafName);

    


    shortcut.setShortcut(target, this.installDir.clone(), null,
                         this.shortDescription, this.iconFile, 0);

    let desktop = Services.dirsvc.get("Desk", Ci.nsILocalFile);
    let startMenu = Services.dirsvc.get("Progs", Ci.nsILocalFile);

    shortcut.copyTo(desktop, this.appNameAsFilename + ".lnk");
    shortcut.copyTo(startMenu, this.appNameAsFilename + ".lnk");

    shortcut.followLinks = false;
    shortcut.remove(false);
  },

  





  useTmpForIcon: false,

  









  processIcon: function(aMimeType, aImageStream, aCallback) {
    let iconStream;
    try {
      let imgTools = Cc["@mozilla.org/image/tools;1"]
                       .createInstance(Ci.imgITools);
      let imgContainer = { value: null };

      imgTools.decodeImageData(aImageStream, aMimeType, imgContainer);
      iconStream = imgTools.encodeImage(imgContainer.value,
                                        "image/vnd.microsoft.icon",
                                        "format=bmp;bpp=32");
    } catch (e) {
      throw("processIcon - Failure converting icon (" + e + ")");
    }

    if (!this.iconFile.parent.exists())
      this.iconFile.parent.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    let outputStream = FileUtils.openSafeFileOutputStream(this.iconFile);
    NetUtil.asyncCopy(iconStream, outputStream);
  }
}

#elifdef XP_MACOSX

function MacNativeApp(aData) {
  NativeApp.call(this, aData);
  this._init();
}

MacNativeApp.prototype = {
  _init: function() {
    this.appSupportDir = Services.dirsvc.get("ULibDir", Ci.nsILocalFile);
    this.appSupportDir.append("Application Support");

    let filenameRE = new RegExp("[<>:\"/\\\\|\\?\\*]", "gi");
    this.appNameAsFilename = this.appNameAsFilename.replace(filenameRE, "");
    if (this.appNameAsFilename == "") {
      this.appNameAsFilename = "Webapp";
    }

    
    
    
    
    this.appProfileDir = this.appSupportDir.clone();
    this.appProfileDir.append(this.launchURI.host + ";" +
                              this.launchURI.scheme + ";" +
                              this.launchURI.port);

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
  },

  install: function() {
    this._removeInstallation(true);
    try {
      this._createDirectoryStructure();
      this._copyPrebuiltFiles();
      this._createConfigFiles();
      this._createAppProfile();
    } catch (ex) {
      this._removeInstallation(false);
      throw(ex);
    }

    getIconForApp(this, this._moveToApplicationsFolder);
  },

  _removeInstallation: function(keepProfile) {
    let filesToRemove = [this.installDir];

    if (!keepProfile) {
      filesToRemove.push(this.appProfileDir);
    }

    removeFiles(filesToRemove);
  },

  _createDirectoryStructure: function() {
    if (!this.appProfileDir.exists())
      this.appProfileDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);

    this.contentsDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    this.macOSDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    this.resourcesDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  },

  _createAppProfile: function() {
    let profSvc = Cc["@mozilla.org/toolkit/profile-service;1"]
                    .getService(Ci.nsIToolkitProfileService);

    try {
      this.appProfile = profSvc.createDefaultProfileForApp(this.appProfileDir.leafName,
                                                           null, null);
    } catch (ex if ex.result == Cr.NS_ERROR_ALREADY_INITIALIZED) {}
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
    <string>' + escapeXML(this.launchURI.prePath) + '</string>\n\
    <key>CFBundleInfoDictionaryVersion</key>\n\
    <string>6.0</string>\n\
    <key>CFBundleName</key>\n\
    <string>' + escapeXML(this.appName) + '</string>\n\
    <key>CFBundlePackageType</key>\n\
    <string>APPL</string>\n\
    <key>CFBundleVersion</key>\n\
    <string>0</string>\n\
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
    let destination = getAvailableFile(appDir,
                                       this.appNameAsFilename,
                                       ".app");
    if (!destination) {
      return false;
    }
    this.installDir.moveTo(destination.parent, destination.leafName);
  },

  





  useTmpForIcon: true,

  









  processIcon: function(aMimeType, aIcon, aCallback) {
    try {
      let process = Cc["@mozilla.org/process/util;1"]
                    .createInstance(Ci.nsIProcess);
      let sipsFile = Cc["@mozilla.org/file/local;1"]
                    .createInstance(Ci.nsILocalFile);
      sipsFile.initWithPath("/usr/bin/sips");

      process.init(sipsFile);
      process.run(true, ["-s",
                  "format", "icns",
                  aIcon.path,
                  "--out", this.iconFile.path,
                  "-z", "128", "128"],
                  9);
    } catch(e) {
      throw(e);
    } finally {
      aCallback.call(this);
    }
  }

}

#elifdef XP_UNIX

function LinuxNativeApp(aData) {
  NativeApp.call(this, aData);
  this._init();
}

LinuxNativeApp.prototype = {
  _init: function() {
    
    
    
    

    this.uniqueName = this.launchURI.scheme + ";" + this.launchURI.host;
    if (this.launchURI.port != -1)
      this.uniqueName += ";" + this.launchURI.port;

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
  },

  install: function() {
    this._removeInstallation(true);

    try {
      this._createDirectoryStructure();
      this._copyPrebuiltFiles();
      this._createConfigFiles();
      this._createAppProfile();
    } catch (ex) {
      this._removeInstallation(false);
      throw(ex);
    }

    getIconForApp(this, function() {});
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

  _createAppProfile: function() {
    let profSvc = Cc["@mozilla.org/toolkit/profile-service;1"]
                    .getService(Ci.nsIToolkitProfileService);

    try {
      this.appProfile = profSvc.createDefaultProfileForApp(this.installDir.leafName,
                                                           null, null);
    } catch (ex if ex.result == Cr.NS_ERROR_ALREADY_INITIALIZED) {}
  },

  






  _translateCategories: function() {
    let translations = {
      "books-reference": "Education;Literature",
      "business": "Finance",
      "education": "Education",
      "entertainment-sports": "Amusement;Sports",
      "games": "Game",
      "health-fitness": "MedicalSoftware",
      "lifestyle": "Amusement",
      "music": "Audio;Music",
      "news-weather": "News",
      "photos-media": "AudioVideo",
      "productivity": "Office",
      "shopping": "Amusement",
      "social": "Chat",
      "travel": "Amusement",
      "utilities": "Utility"
    };

    
    let categories = "";
    for (let category of this.app.categories) {
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

  





  useTmpForIcon: false,

  








  processIcon: function(aMimeType, aImageStream, aCallback) {
    let iconStream;
    try {
      let imgTools = Cc["@mozilla.org/image/tools;1"]
                       .createInstance(Ci.imgITools);
      let imgContainer = { value: null };

      imgTools.decodeImageData(aImageStream, aMimeType, imgContainer);
      iconStream = imgTools.encodeImage(imgContainer.value, "image/png");
    } catch (e) {
      throw("processIcon - Failure converting icon (" + e + ")");
    }

    let outputStream = FileUtils.openSafeFileOutputStream(this.iconFile);
    NetUtil.asyncCopy(iconStream, outputStream);
  }
}

#endif









function writeToFile(aFile, aData) {
  let path = aFile.path;
  let data = new TextEncoder().encode(aData);
  return OS.File.writeAtomic(path, data, { tmpPath: path + ".tmp" });
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











function getAvailableFile(aFolder, aName, aExtension) {
  let folder = aFolder.QueryInterface(Ci.nsILocalFile);
  folder.followLinks = false;
  if (!folder.isDirectory() || !folder.isWritable()) {
    return null;
  }

  let file = folder.clone();
  file.append(aName + aExtension);

  if (!file.exists()) {
    return file;
  }

  for (let i = 2; i < 10; i++) {
    file.leafName = aName + " (" + i + ")" + aExtension;
    if (!file.exists()) {
      return file;
    }
  }

  for (let i = 10; i < 100; i++) {
    file.leafName = aName + "-" + i + aExtension;
    if (!file.exists()) {
      return file;
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
