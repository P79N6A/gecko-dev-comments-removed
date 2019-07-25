



let EXPORTED_SYMBOLS = ["WebappsInstaller"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/FileUtils.jsm");
Cu.import("resource://gre/modules/NetUtil.jsm");

let WebappsInstaller = {
  






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
#else
    return false;
#endif

    try {
      shell.install();
    } catch (ex) {
      Cu.reportError("Error installing app: " + ex);
      return false;
    }

    return true;
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

  this.manifest = app.manifest;

  this.profileFolder = Services.dirsvc.get("ProfD", Ci.nsIFile);
}

#ifdef XP_WIN






























function WinNativeApp(aData) {
  NativeApp.call(this, aData);
  this._init();
}

WinNativeApp.prototype = {
  



  install: function() {
    
    this._removeInstallation();

    try {
      this._createDirectoryStructure();
      this._copyPrebuiltFiles();
      this._createConfigFiles();
      this._createShortcutFiles();
      this._writeSystemKeys();
    } catch (ex) {
      this._removeInstallation();
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
    this.installDir.append(this.launchURI.host + ";" + 
                           this.launchURI.scheme + ";" +
                           this.launchURI.port);

    this.uninstallDir = this.installDir.clone();
    this.uninstallDir.append("uninstall");

    this.uninstallerFile = this.uninstallDir.clone();
    this.uninstallerFile.append("webapp-uninstaller.exe");

    this.iconFile = this.installDir.clone();
    this.iconFile.append("chrome");
    this.iconFile.append("icons");
    this.iconFile.append("default");
    this.iconFile.append("default.ico");

    this.processFolder = Services.dirsvc.get("CurProcD", Ci.nsIFile);

    this.desktopShortcut = Services.dirsvc.get("Desk", Ci.nsILocalFile);
    this.desktopShortcut.append(this.appNameAsFilename + ".lnk");
    this.desktopShortcut.followLinks = false;

    this.startMenuShortcut = Services.dirsvc.get("Progs", Ci.nsILocalFile);
    this.startMenuShortcut.append(this.appNameAsFilename + ".lnk");
    this.startMenuShortcut.followLinks = false;

    this.uninstallSubkeyStr = this.launchURI.scheme + "://" +
                              this.launchURI.host + ":" +
                              this.launchURI.port;
  },

  


  _removeInstallation : function() {
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
    } finally {
      if(uninstallKey)
        uninstallKey.close();
    }

    try {
      if(this.installDir.exists()) {
        let dir = this.installDir.QueryInterface(Ci.nsILocalFile);
        
        
        
        dir.followLinks = false;
        dir.remove(true);
      }
    } catch(ex) {
    }

    try {
      if(this.desktopShortcut && this.desktopShortcut.exists()) {
        this.desktopShortcut.remove(false);
      }

      if(this.startMenuShortcut && this.startMenuShortcut.exists()) {
        this.startMenuShortcut.remove(false);
      }
    } catch(ex) {
    }
  },

  


  _createDirectoryStructure: function() {
    this.installDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    this.uninstallDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  },

  


  _copyPrebuiltFiles: function() {
    let webapprt = this.processFolder.clone();
    webapprt.append("webapprt-stub.exe");
    webapprt.copyTo(this.installDir, this.appNameAsFilename + ".exe");

    let uninstaller = this.processFolder.clone();
    uninstaller.append("webapp-uninstaller.exe");
    uninstaller.copyTo(this.uninstallDir, this.uninstallerFile.leafName);
  },

  


  _createConfigFiles: function() {
    
    let json = {
      "registryDir": this.profileFolder.path,
      "app": this.app
    };

    let configJson = this.installDir.clone();
    configJson.append("webapp.json");
    writeToFile(configJson, JSON.stringify(json), function() {});

    
    let webappINI = this.installDir.clone().QueryInterface(Ci.nsILocalFile);
    webappINI.append("webapp.ini");

    let factory = Cc["@mozilla.org/xpcom/ini-processor-factory;1"]
                    .getService(Ci.nsIINIParserFactory);

    let writer = factory.createINIParser(webappINI).QueryInterface(Ci.nsIINIParserWriter);
    writer.setString("Webapp", "Name", this.appName);
    writer.setString("Webapp", "Profile", this.installDir.leafName);
    writer.setString("Webapp", "Executable", this.appNameAsFilename);
    writer.setString("WebappRT", "InstallDir", this.processFolder.path);
    writer.writeFile();

    
    let shortcutLogsINI = this.uninstallDir.clone().QueryInterface(Ci.nsILocalFile);
    shortcutLogsINI.append("shortcuts_log.ini");

    writer = factory.createINIParser(shortcutLogsINI).QueryInterface(Ci.nsIINIParserWriter);
    writer.setString("STARTMENU", "Shortcut0", this.appNameAsFilename + ".lnk");
    writer.setString("DESKTOP", "Shortcut0", this.appNameAsFilename + ".lnk");
    writer.setString("TASKBAR", "Migrated", "true");
    writer.writeFile();

    writer = null;
    factory = null;

    
    let uninstallContent = 
      "File: \\webapp.ini\r\n" +
      "File: \\webapp.json\r\n" +
      "File: \\webapprt.old\r\n" +
      "File: \\chrome\\icons\\default\\default.ico";
    let uninstallLog = this.uninstallDir.clone();
    uninstallLog.append("uninstall.log");
    writeToFile(uninstallLog, uninstallContent, function() {});
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
    target.append(this.appNameAsFilename + ".exe");

    


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

    this.installDir = Services.dirsvc.get("LocApp", Ci.nsILocalFile);
    this.installDir.append(this.appNameAsFilename + ".app");

    this.contentsDir = this.installDir.clone();
    this.contentsDir.append("Contents");

    this.macOSDir = this.contentsDir.clone();
    this.macOSDir.append("MacOS");

    this.resourcesDir = this.contentsDir.clone();
    this.resourcesDir.append("Resources");

    this.iconFile = this.resourcesDir.clone();
    this.iconFile.append("appicon.icns");

    this.processFolder = Services.dirsvc.get("CurProcD", Ci.nsIFile);
  },

  install: function() {
    this._removeInstallation(true);
    try {
      this._createDirectoryStructure();
      this._copyPrebuiltFiles();
      this._createConfigFiles();
    } catch (ex) {
      this._removeInstallation(false);
      throw(ex);
    }

    getIconForApp(this, this._createPListFile);
  },

  _removeInstallation: function(keepProfile) {
    try {
      if(this.installDir.exists()) {
        this.installDir.followLinks = false;
        this.installDir.remove(true);
      }
    } catch(ex) {
    }

   if (keepProfile)
     return;

   try {
      if(this.appProfileDir.exists()) {
        this.appProfileDir.followLinks = false;
        this.appProfileDir.remove(true);
      }
    } catch(ex) {
    }
  },

  _createDirectoryStructure: function() {
    if (!this.appProfileDir.exists())
      this.appProfileDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);

    this.installDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    this.contentsDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    this.macOSDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
    this.resourcesDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  },

  _copyPrebuiltFiles: function() {
    let webapprt = this.processFolder.clone();
    webapprt.append("webapprt-stub");
    webapprt.copyTo(this.macOSDir, "webapprt");
  },

  _createConfigFiles: function() {
    
    let json = {
      "registryDir": this.profileFolder.path,
      "app": {
        "origin": this.launchURI.prePath,
        "installOrigin": "apps.mozillalabs.com",
        "manifest": this.manifest
       }
    };

    let configJson = this.appProfileDir.clone();
    configJson.append("webapp.json");
    writeToFile(configJson, JSON.stringify(json), function() {});

    
    let applicationINI = this.macOSDir.clone().QueryInterface(Ci.nsILocalFile);
    applicationINI.append("webapp.ini");

    let factory = Cc["@mozilla.org/xpcom/ini-processor-factory;1"]
                    .getService(Ci.nsIINIParserFactory);

    let writer = factory.createINIParser(applicationINI).QueryInterface(Ci.nsIINIParserWriter);
    writer.setString("Webapp", "Name", this.appName);
    writer.setString("Webapp", "Profile", this.appProfileDir.leafName);
    writer.writeFile();
  },

  _createPListFile: function() {
    
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
    <key>CFBundleSignature</key>\n\
    <string>MOZB</string>\n\
    <key>CFBundleVersion</key>\n\
    <string>0</string>\n\
#ifdef DEBUG
    <key>FirefoxBinary</key>\n\
    <string>org.mozilla.NightlyDebug</string>\n\
#endif
  </dict>\n\
</plist>';

    let infoPListFile = this.contentsDir.clone();
    infoPListFile.append("Info.plist");
    writeToFile(infoPListFile, infoPListContent, function() {});
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

#endif










function writeToFile(aFile, aData, aCallback) {
  let ostream = FileUtils.openSafeFileOutputStream(aFile);
  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"]
                    .createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  let istream = converter.convertToInputStream(aData);
  NetUtil.asyncCopy(istream, ostream, function(x) aCallback(x));
}




function sanitize(aStr) {
  let unprintableRE = new RegExp("[\\x00-\\x1F\\x7F]" ,"gi");
  return aStr.replace(unprintableRE, "");
}




function stripStringForFilename(aPossiblyBadFilenameString) {
  

  let stripFrontRE = new RegExp("^\\W*","gi");
  let stripBackRE = new RegExp("\\W*$","gi");

  let stripped = aPossiblyBadFilenameString.replace(stripFrontRE, "");
  stripped = stripped.replace(stripBackRE, "");
  return stripped;
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
