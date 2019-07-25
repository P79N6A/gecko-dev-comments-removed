








































const AUS_Cc = Components.classes;
const AUS_Ci = Components.interfaces;
const AUS_Cr = Components.results;
const AUS_Cu = Components.utils;

const PREF_APP_UPDATE_CERTS_BRANCH        = "app.update.certs.";
const PREF_APP_UPDATE_CERT_CHECKATTRS     = "app.update.cert.checkAttributes";
const PREF_APP_UPDATE_CERT_ERRORS         = "app.update.cert.errors";
const PREF_APP_UPDATE_CERT_MAXERRORS      = "app.update.cert.maxErrors";
const PREF_APP_UPDATE_CERT_REQUIREBUILTIN = "app.update.cert.requireBuiltIn";
const PREF_APP_UPDATE_CHANNEL             = "app.update.channel";
const PREF_APP_UPDATE_ENABLED             = "app.update.enabled";
const PREF_APP_UPDATE_IDLETIME            = "app.update.idletime";
const PREF_APP_UPDATE_LOG                 = "app.update.log";
const PREF_APP_UPDATE_NEVER_BRANCH        = "app.update.never.";
const PREF_APP_UPDATE_PROMPTWAITTIME      = "app.update.promptWaitTime";
const PREF_APP_UPDATE_SHOW_INSTALLED_UI   = "app.update.showInstalledUI";
const PREF_APP_UPDATE_URL                 = "app.update.url";
const PREF_APP_UPDATE_URL_DETAILS         = "app.update.url.details";
const PREF_APP_UPDATE_URL_OVERRIDE        = "app.update.url.override";

const PREF_APP_UPDATE_CERT_INVALID_ATTR_NAME = PREF_APP_UPDATE_CERTS_BRANCH +
                                               "1.invalidName";

const PREF_APP_PARTNER_BRANCH             = "app.partner.";
const PREF_DISTRIBUTION_ID                = "distribution.id";
const PREF_DISTRIBUTION_VERSION           = "distribution.version";

const PREF_EXTENSIONS_UPDATE_URL          = "extensions.update.url";

const NS_APP_PROFILE_DIR_STARTUP   = "ProfDS";
const NS_APP_USER_PROFILE_50_DIR   = "ProfD";
const NS_GRE_DIR                   = "GreD";
const NS_XPCOM_CURRENT_PROCESS_DIR = "XCurProcD";
const XRE_UPDATE_ROOT_DIR          = "UpdRootD";

const STATE_NONE            = "null";
const STATE_DOWNLOADING     = "downloading";
const STATE_PENDING         = "pending";
const STATE_APPLYING        = "applying";
const STATE_SUCCEEDED       = "succeeded";
const STATE_DOWNLOAD_FAILED = "download-failed";
const STATE_FAILED          = "failed";

const FILE_BACKUP_LOG     = "backup-update.log";
const FILE_LAST_LOG       = "last-update.log";
const FILE_UPDATES_DB     = "updates.xml";
const FILE_UPDATE_ACTIVE  = "active-update.xml";
const FILE_UPDATE_ARCHIVE = "update.mar";
const FILE_UPDATE_LOG     = "update.log";
const FILE_UPDATE_STATUS  = "update.status";

const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;

const PR_RDWR        = 0x04;
const PR_CREATE_FILE = 0x08;
const PR_APPEND      = 0x10;
const PR_TRUNCATE    = 0x20;
const PR_SYNC        = 0x40;
const PR_EXCL        = 0x80;

const PERMS_FILE      = 0644;
const PERMS_DIRECTORY = 0755;

AUS_Cu.import("resource://gre/modules/Services.jsm");

const URI_UPDATES_PROPERTIES = "chrome://mozapps/locale/update/updates.properties";
const gUpdateBundle = Services.strings.createBundle(URI_UPDATES_PROPERTIES);


__defineGetter__("gAUS", function() {
  delete this.gAUS;
  return this.gAUS = AUS_Cc["@mozilla.org/updates/update-service;1"].
                     getService(AUS_Ci.nsIApplicationUpdateService).
                     QueryInterface(AUS_Ci.nsITimerCallback).
                     QueryInterface(AUS_Ci.nsIObserver);
});

__defineGetter__("gUpdateManager", function() {
  delete this.gUpdateManager;
  return this.gUpdateManager = AUS_Cc["@mozilla.org/updates/update-manager;1"].
                               getService(AUS_Ci.nsIUpdateManager);
});

__defineGetter__("gUpdateChecker", function() {
  delete this.gUpdateChecker;
  return this.gUpdateChecker = AUS_Cc["@mozilla.org/updates/update-checker;1"].
                               createInstance(AUS_Ci.nsIUpdateChecker);
});

__defineGetter__("gUP", function() {
  delete this.gUP;
  return this.gUP = AUS_Cc["@mozilla.org/updates/update-prompt;1"].
                    createInstance(AUS_Ci.nsIUpdatePrompt);
});

__defineGetter__("gDefaultPrefBranch", function() {
  delete this.gDefaultPrefBranch;
  return this.gDefaultPrefBranch = Services.prefs.getDefaultBranch(null);
});

__defineGetter__("gZipW", function() {
  delete this.gZipW;
  return this.gZipW = AUS_Cc["@mozilla.org/zipwriter;1"].
                      createInstance(AUS_Ci.nsIZipWriter);
});


function initUpdateServiceStub() {
  AUS_Cc["@mozilla.org/updates/update-service-stub;1"].
  createInstance(AUS_Ci.nsISupports);
}


function reloadUpdateManagerData() {
  gUpdateManager.QueryInterface(AUS_Ci.nsIObserver).
  observe(null, "um-reload-update-data", "");
}






function setUpdateChannel(aChannel) {
  gDefaultPrefBranch.setCharPref(PREF_APP_UPDATE_CHANNEL,
                                 aChannel ? aChannel : "test_channel");
}







function setUpdateURLOverride(aURL) {
  Services.prefs.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE,
                             aURL ? aURL : URL_HOST + "update.xml");
}







function getRemoteUpdatesXMLString(aUpdates) {
  return "<?xml version=\"1.0\"?>\n" +
         "<updates>\n" +
           aUpdates +
         "</updates>\n";
}







function getRemoteUpdateString(aPatches, aType, aName, aDisplayVersion,
                               aAppVersion, aPlatformVersion, aBuildID,
                               aDetailsURL, aBillboardURL, aLicenseURL,
                               aShowPrompt, aShowNeverForVersion, aShowSurvey,
                               aVersion, aExtensionVersion, aCustom1,
                               aCustom2) {
  return  getUpdateString(aType, aName, aDisplayVersion, aAppVersion,
                          aPlatformVersion, aBuildID, aDetailsURL,
                          aBillboardURL, aLicenseURL, aShowPrompt,
                          aShowNeverForVersion, aShowSurvey, aVersion,
                          aExtensionVersion, aCustom1, aCustom2) + ">\n" +
              aPatches + 
         "  </update>\n";
}








function getRemotePatchString(aType, aURL, aHashFunction, aHashValue, aSize) {
  return getPatchString(aType, aURL, aHashFunction, aHashValue, aSize) +
         "/>\n";
}







function getLocalUpdatesXMLString(aUpdates) {
  if (!aUpdates || aUpdates == "")
    return "<updates xmlns=\"http://www.mozilla.org/2005/app-update\"/>"
  return ("<updates xmlns=\"http://www.mozilla.org/2005/app-update\">" +
           aUpdates +
         "</updates>").replace(/>\s+\n*</g,'><');
}
























function getLocalUpdateString(aPatches, aType, aName, aDisplayVersion,
                              aAppVersion, aPlatformVersion, aBuildID,
                              aDetailsURL, aBillboardURL, aLicenseURL,
                              aServiceURL, aInstallDate, aStatusText,
                              aIsCompleteUpdate, aChannel, aForegroundDownload,
                              aShowPrompt, aShowNeverForVersion, aShowSurvey,
                              aVersion, aExtensionVersion, aPreviousAppVersion,
                              aCustom1, aCustom2) {
  var serviceURL = aServiceURL ? aServiceURL : "http://test_service/";
  var installDate = aInstallDate ? aInstallDate : "1238441400314";
  var statusText = aStatusText ? aStatusText : "Install Pending";
  var isCompleteUpdate = typeof(aIsCompleteUpdate) == "string" ? aIsCompleteUpdate : "true";
  var channel = aChannel ? aChannel : "test_channel";
  var foregroundDownload =
    typeof(aForegroundDownload) == "string" ? aForegroundDownload : "true";
  var previousAppVersion = aPreviousAppVersion ? "previousAppVersion=\"" + aPreviousAppVersion + "\" " : "";
  return getUpdateString(aType, aName, aDisplayVersion, aAppVersion,
                         aPlatformVersion, aBuildID, aDetailsURL, aBillboardURL,
                         aLicenseURL, aShowPrompt, aShowNeverForVersion,
                         aShowSurvey, aVersion, aExtensionVersion, aCustom1,
                         aCustom2) +
                   " " +
                   previousAppVersion +
                   "serviceURL=\"" + serviceURL + "\" " +
                   "installDate=\"" + installDate + "\" " +
                   "statusText=\"" + statusText + "\" " +
                   "isCompleteUpdate=\"" + isCompleteUpdate + "\" " +
                   "channel=\"" + channel + "\" " +
                   "foregroundDownload=\"" + foregroundDownload + "\">"  +
              aPatches + 
         "  </update>";
}













function getLocalPatchString(aType, aURL, aHashFunction, aHashValue, aSize,
                             aSelected, aState) {
  var selected = typeof(aSelected) == "string" ? aSelected : "true";
  var state = aState ? aState : STATE_SUCCEEDED;
  return getPatchString(aType, aURL, aHashFunction, aHashValue, aSize) + " " +
         "selected=\"" + selected + "\" " +
         "state=\"" + state + "\"/>\n";
}


























































function getUpdateString(aType, aName, aDisplayVersion, aAppVersion,
                         aPlatformVersion, aBuildID, aDetailsURL, aBillboardURL,
                         aLicenseURL, aShowPrompt, aShowNeverForVersion,
                         aShowSurvey, aVersion, aExtensionVersion, aCustom1,
                         aCustom2) {
  var type = aType ? aType : "major";
  var name = aName ? aName : "App Update Test";
  var displayVersion = "";
  if (aDisplayVersion || !aVersion) {
    displayVersion = "displayVersion=\"" +
                     (aDisplayVersion ? aDisplayVersion
                                      : "version 99.0") + "\" ";
  }
  
  
  var version = aVersion ? "version=\"" + aVersion + "\" " : "";
  var appVersion = "";
  if (aAppVersion || !aExtensionVersion) {
    appVersion = "appVersion=\"" + (aAppVersion ? aAppVersion : "99.0") + "\" ";
  }
  
  
  var extensionVersion = aExtensionVersion ? "extensionVersion=\"" + aExtensionVersion + "\" " : "";
  var platformVersion = "";
  if (aPlatformVersion) {
    platformVersion = "platformVersion=\"" + (aPlatformVersion ? aPlatformVersion : "99.0") + "\" ";
  }
  var buildID = aBuildID ? aBuildID : "20080811053724";
  

  var detailsURL = "detailsURL=\"" + (aDetailsURL ? aDetailsURL : "http://test_details/") + "\" ";
  var billboardURL = aBillboardURL ? "billboardURL=\"" + aBillboardURL + "\" " : "";
  var licenseURL = aLicenseURL ? "licenseURL=\"" + aLicenseURL + "\" " : "";
  var showPrompt = aShowPrompt ? "showPrompt=\"" + aShowPrompt + "\" " : "";
  var showNeverForVersion = aShowNeverForVersion ? "showNeverForVersion=\"" + aShowNeverForVersion + "\" " : "";
  var showSurvey = aShowSurvey ? "showSurvey=\"" + aShowSurvey + "\" " : "";
  var custom1 = aCustom1 ? aCustom1 + " " : "";
  var custom2 = aCustom2 ? aCustom2 + " " : "";
  return "  <update type=\"" + type + "\" " +
                   "name=\"" + name + "\" " +
                   displayVersion +
                   version +
                   appVersion +
                   extensionVersion +
                   platformVersion +
                   detailsURL +
                   billboardURL +
                   licenseURL +
                   showPrompt +
                   showNeverForVersion +
                   showSurvey +
                   custom1 +
                   custom2 +
                   "buildID=\"" + buildID + "\"";
}






















function getPatchString(aType, aURL, aHashFunction, aHashValue, aSize) {
  var type = aType ? aType : "complete";
  var url = aURL ? aURL : URL_HOST + URL_PATH + "/empty.mar";
  var hashFunction = aHashFunction ? aHashFunction : "MD5";
  var hashValue = aHashValue ? aHashValue : "6232cd43a1c77e30191c53a329a3f99d";
  var size = aSize ? aSize : "775";
  return "    <patch type=\"" + type + "\" " +
                     "URL=\"" + url + "\" " +
                     "hashFunction=\"" + hashFunction + "\" " +
                     "hashValue=\"" + hashValue + "\" " +
                     "size=\"" + size + "\"";
}










function writeUpdatesToXMLFile(aContent, aIsActiveUpdate) {
  var file = getCurrentProcessDir();
  file.append(aIsActiveUpdate ? FILE_UPDATE_ACTIVE : FILE_UPDATES_DB);
  writeFile(file, aContent);
}








function writeStatusFile(aStatus) {
  var file = getUpdatesDir();
  file.append("0");
  file.append(FILE_UPDATE_STATUS);
  aStatus += "\n";
  writeFile(file, aStatus);
}





function getUpdatesDir() {
  var dir = getCurrentProcessDir();
  dir.append("updates");
  return dir;
}










function writeFile(aFile, aText) {
  var fos = AUS_Cc["@mozilla.org/network/file-output-stream;1"].
            createInstance(AUS_Ci.nsIFileOutputStream);
  if (!aFile.exists())
    aFile.create(AUS_Ci.nsILocalFile.NORMAL_FILE_TYPE, PERMS_FILE);
  fos.init(aFile, MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE, PERMS_FILE, 0);
  fos.write(aText, aText.length);
  fos.close();
}







function readFile(aFile) {
  var fis = AUS_Cc["@mozilla.org/network/file-input-stream;1"].
            createInstance(AUS_Ci.nsIFileInputStream);
  if (!aFile.exists())
    return null;
  fis.init(aFile, MODE_RDONLY, PERMS_FILE, 0);
  var sis = AUS_Cc["@mozilla.org/scriptableinputstream;1"].
            createInstance(AUS_Ci.nsIScriptableInputStream);
  sis.init(fis);
  var text = sis.read(sis.available());
  sis.close();
  return text;
}







function readFileBytes(aFile) {
  var fis = AUS_Cc["@mozilla.org/network/file-input-stream;1"].
            createInstance(AUS_Ci.nsIFileInputStream);
  fis.init(aFile, -1, -1, false);
   var bis = AUS_Cc["@mozilla.org/binaryinputstream;1"].
             createInstance(AUS_Ci.nsIBinaryInputStream);
   bis.setInputStream(fis);
   var data = [];
   var count = fis.available();
   while (count > 0) {
     var bytes = bis.readByteArray(Math.min(65535, count));
     data.push(String.fromCharCode.apply(null, bytes));
     count -= bytes.length;
     if (bytes.length == 0)
       do_throw("Nothing read from input stream!");
   }
  data.join('');
  fis.close();
  return data.toString();
}


function getStatusText(aErrCode) {
  return getString("check_error-" + aErrCode);
}


function getString(aName) {
  try {
    return gUpdateBundle.GetStringFromName(aName);
  }
  catch (e) {
  }
  return null;
}







function getFileExtension(aFile) {
  return Services.io.newFileURI(aFile).QueryInterface(AUS_Ci.nsIURL).
         fileExtension;
}







function removeUpdateDirsAndFiles() {
  var appDir = getCurrentProcessDir();
  var file = appDir.clone();
  file.append(FILE_UPDATE_ACTIVE);
  try {
    if (file.exists())
      file.remove(false);
  }
  catch (e) {
    dump("Unable to remove file\npath: " + file.path +
         "\nException: " + e + "\n");
  }

  file = appDir.clone();
  file.append(FILE_UPDATES_DB);
  try {
    if (file.exists())
      file.remove(false);
  }
  catch (e) {
    dump("Unable to remove file\npath: " + file.path +
         "\nException: " + e + "\n");
  }

  
  var updatesDir = appDir.clone();
  updatesDir.append("updates");
  try {
    cleanUpdatesDir(updatesDir);
  }
  catch (e) {
    dump("Unable to remove files / directories from directory\npath: " +
         updatesDir.path + "\nException: " + e + "\n");
  }
}







function cleanUpdatesDir(aDir) {
  if (!aDir.exists())
    return;

  var dirEntries = aDir.directoryEntries;
  while (dirEntries.hasMoreElements()) {
    var entry = dirEntries.getNext().QueryInterface(AUS_Ci.nsIFile);

    if (entry.isDirectory()) {
      if (entry.leafName == "0" && entry.parent.leafName == "updates") {
        cleanUpdatesDir(entry);
        entry.permissions = PERMS_DIRECTORY;
      }
      else {
        try {
          entry.remove(true);
          return;
        }
        catch (e) {
        }
        cleanUpdatesDir(entry);
        entry.permissions = PERMS_DIRECTORY;
        entry.remove(true);
      }
    }
    else {
      entry.permissions = PERMS_FILE;
      entry.remove(false);
    }
  }
}








function removeDirRecursive(aDir) {
  if (!aDir.exists())
    return;
  try {
    aDir.remove(true);
    return;
  }
  catch (e) {
  }

  var dirEntries = aDir.directoryEntries;
  while (dirEntries.hasMoreElements()) {
    var entry = dirEntries.getNext().QueryInterface(AUS_Ci.nsIFile);

    if (entry.isDirectory()) {
      removeDirRecursive(entry);
    }
    else {
      entry.permissions = PERMS_FILE;
      entry.remove(false);
    }
  }
  aDir.permissions = PERMS_DIRECTORY;
  aDir.remove(true);
}






function getCurrentProcessDir() {
  return Services.dirsvc.get(NS_XPCOM_CURRENT_PROCESS_DIR, AUS_Ci.nsIFile);
}







function getGREDir() {
  return Services.dirsvc.get(NS_GRE_DIR, AUS_Ci.nsIFile);
}
