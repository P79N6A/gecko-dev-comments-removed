



let EXPORTED_SYMBOLS = ["WebappsInstaller"];

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cu = Components.utils;

Cu.import("resource:///modules/Services.jsm");
Cu.import("resource:///modules/FileUtils.jsm");
Cu.import("resource:///modules/NetUtil.jsm");

let WebappsInstaller = {
  






  install: function(aData) {

#ifdef XP_MACOSX
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

#ifdef XP_MACOSX

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

    getIconForApp(this);
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
      "app": {
        "profile": this.profileFolder.path,
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
    writer.setString("Branding", "BrandFullName", this.appName);
    writer.setString("Branding", "BrandShortName", this.appName);
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

  









  processIcon: function(aMimeType, aIcon) {
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
