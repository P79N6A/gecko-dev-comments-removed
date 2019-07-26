







const AUS_Cc = Components.classes;
const AUS_Ci = Components.interfaces;
const AUS_Cr = Components.results;
const AUS_Cu = Components.utils;
const AUS_Cm = Components.manager;

const PREF_APP_UPDATE_AUTO                = "app.update.auto";
const PREF_APP_UPDATE_BACKGROUNDERRORS    = "app.update.backgroundErrors";
const PREF_APP_UPDATE_BACKGROUNDMAXERRORS = "app.update.backgroundMaxErrors";
const PREF_APP_UPDATE_CERTS_BRANCH        = "app.update.certs.";
const PREF_APP_UPDATE_CERT_CHECKATTRS     = "app.update.cert.checkAttributes";
const PREF_APP_UPDATE_CERT_ERRORS         = "app.update.cert.errors";
const PREF_APP_UPDATE_CERT_MAXERRORS      = "app.update.cert.maxErrors";
const PREF_APP_UPDATE_CERT_REQUIREBUILTIN = "app.update.cert.requireBuiltIn";
const PREF_APP_UPDATE_CHANNEL             = "app.update.channel";
const PREF_APP_UPDATE_ENABLED             = "app.update.enabled";
const PREF_APP_UPDATE_METRO_ENABLED       = "app.update.metro.enabled";
const PREF_APP_UPDATE_IDLETIME            = "app.update.idletime";
const PREF_APP_UPDATE_LOG                 = "app.update.log";
const PREF_APP_UPDATE_NEVER_BRANCH        = "app.update.never.";
const PREF_APP_UPDATE_NOTIFIEDUNSUPPORTED = "app.update.notifiedUnsupported";
const PREF_APP_UPDATE_PROMPTWAITTIME      = "app.update.promptWaitTime";
const PREF_APP_UPDATE_SERVICE_ENABLED     = "app.update.service.enabled";
const PREF_APP_UPDATE_SHOW_INSTALLED_UI   = "app.update.showInstalledUI";
const PREF_APP_UPDATE_SILENT              = "app.update.silent";
const PREF_APP_UPDATE_STAGING_ENABLED     = "app.update.staging.enabled";
const PREF_APP_UPDATE_URL                 = "app.update.url";
const PREF_APP_UPDATE_URL_DETAILS         = "app.update.url.details";
const PREF_APP_UPDATE_URL_OVERRIDE        = "app.update.url.override";
const PREF_APP_UPDATE_SOCKET_ERRORS       = "app.update.socket.maxErrors";
const PREF_APP_UPDATE_RETRY_TIMEOUT       = "app.update.socket.retryTimeout";

const PREF_APP_UPDATE_CERT_INVALID_ATTR_NAME = PREF_APP_UPDATE_CERTS_BRANCH +
                                               "1.invalidName";

const PREF_APP_PARTNER_BRANCH             = "app.partner.";
const PREF_DISTRIBUTION_ID                = "distribution.id";
const PREF_DISTRIBUTION_VERSION           = "distribution.version";

const PREF_EXTENSIONS_UPDATE_URL          = "extensions.update.url";
const PREF_EXTENSIONS_STRICT_COMPAT       = "extensions.strictCompatibility";

const NS_APP_PROFILE_DIR_STARTUP   = "ProfDS";
const NS_APP_USER_PROFILE_50_DIR   = "ProfD";
const NS_GRE_DIR                   = "GreD";
const NS_XPCOM_CURRENT_PROCESS_DIR = "XCurProcD";
const XRE_EXECUTABLE_FILE          = "XREExeF";
const XRE_UPDATE_ROOT_DIR          = "UpdRootD";

const CRC_ERROR   = 4;
const WRITE_ERROR = 7;

const DIR_PATCH        = "0";
const DIR_TOBEDELETED  = "tobedeleted";
const DIR_UPDATES      = "updates";
#ifdef XP_MACOSX
const DIR_BIN_REL_PATH = "Contents/MacOS/";
const DIR_UPDATED      = "Updated.app";
#else
const DIR_BIN_REL_PATH = "";
const DIR_UPDATED      = "updated";
#endif

const FILE_BACKUP_LOG                = "backup-update.log";
const FILE_LAST_LOG                  = "last-update.log";
const FILE_UPDATER_INI               = "updater.ini";
const FILE_UPDATES_DB                = "updates.xml";
const FILE_UPDATE_ACTIVE             = "active-update.xml";
const FILE_UPDATE_ARCHIVE            = "update.mar";
const FILE_UPDATE_LOG                = "update.log";
const FILE_UPDATE_SETTINGS_INI       = "update-settings.ini";
const FILE_UPDATE_SETTINGS_INI_BAK   = "update-settings.ini.bak";
const FILE_UPDATE_STATUS             = "update.status";
const FILE_UPDATE_VERSION            = "update.version";

const UPDATE_SETTINGS_CONTENTS = "[Settings]\n" +
                                 "ACCEPTED_MAR_CHANNEL_IDS=xpcshell-test\n"

const PR_RDWR        = 0x04;
const PR_CREATE_FILE = 0x08;
const PR_APPEND      = 0x10;
const PR_TRUNCATE    = 0x20;
const PR_SYNC        = 0x40;
const PR_EXCL        = 0x80;

const DEFAULT_UPDATE_VERSION = "999999.0";

var gChannel;

#include sharedUpdateXML.js

AUS_Cu.import("resource://gre/modules/FileUtils.jsm");
AUS_Cu.import("resource://gre/modules/Services.jsm");
AUS_Cu.import("resource://gre/modules/XPCOMUtils.jsm");

const PERMS_FILE      = FileUtils.PERMS_FILE;
const PERMS_DIRECTORY = FileUtils.PERMS_DIRECTORY;

const MODE_RDONLY   = FileUtils.MODE_RDONLY;
const MODE_WRONLY   = FileUtils.MODE_WRONLY;
const MODE_RDWR     = FileUtils.MODE_RDWR;
const MODE_CREATE   = FileUtils.MODE_CREATE;
const MODE_APPEND   = FileUtils.MODE_APPEND;
const MODE_TRUNCATE = FileUtils.MODE_TRUNCATE;

const URI_UPDATES_PROPERTIES = "chrome://mozapps/locale/update/updates.properties";
const gUpdateBundle = Services.strings.createBundle(URI_UPDATES_PROPERTIES);

XPCOMUtils.defineLazyGetter(this, "gAUS", function test_gAUS() {
  return AUS_Cc["@mozilla.org/updates/update-service;1"].
         getService(AUS_Ci.nsIApplicationUpdateService).
         QueryInterface(AUS_Ci.nsITimerCallback).
         QueryInterface(AUS_Ci.nsIObserver).
         QueryInterface(AUS_Ci.nsIUpdateCheckListener);
});

XPCOMUtils.defineLazyServiceGetter(this, "gUpdateManager",
                                   "@mozilla.org/updates/update-manager;1",
                                   "nsIUpdateManager");

XPCOMUtils.defineLazyGetter(this, "gUpdateChecker", function test_gUC() {
  return AUS_Cc["@mozilla.org/updates/update-checker;1"].
         createInstance(AUS_Ci.nsIUpdateChecker);
});

XPCOMUtils.defineLazyGetter(this, "gUP", function test_gUP() {
  return AUS_Cc["@mozilla.org/updates/update-prompt;1"].
         createInstance(AUS_Ci.nsIUpdatePrompt);
});

XPCOMUtils.defineLazyGetter(this, "gDefaultPrefBranch", function test_gDPB() {
  return Services.prefs.getDefaultBranch(null);
});

XPCOMUtils.defineLazyGetter(this, "gPrefRoot", function test_gPR() {
  return Services.prefs.getBranch(null);
});

XPCOMUtils.defineLazyGetter(this, "gZipW", function test_gZipW() {
  return AUS_Cc["@mozilla.org/zipwriter;1"].
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
  gChannel = aChannel;
  debugDump("setting default pref " + PREF_APP_UPDATE_CHANNEL + " to " + gChannel);
  gDefaultPrefBranch.setCharPref(PREF_APP_UPDATE_CHANNEL, gChannel);
  gPrefRoot.addObserver(PREF_APP_UPDATE_CHANNEL, observer, false);
}

var observer = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == "nsPref:changed" && aData == PREF_APP_UPDATE_CHANNEL) {
      var channel = gDefaultPrefBranch.getCharPref(PREF_APP_UPDATE_CHANNEL);
      if (channel != gChannel) {
        debugDump("Changing channel from " + channel + " to " + gChannel);
        gDefaultPrefBranch.setCharPref(PREF_APP_UPDATE_CHANNEL, gChannel);
      }
    }
  },
  QueryInterface: XPCOMUtils.generateQI([AUS_Ci.nsIObserver])
};








function setUpdateURLOverride(aURL) {
  let url = aURL ? aURL : URL_HOST + "/update.xml";
  debugDump("setting " + PREF_APP_UPDATE_URL_OVERRIDE + " to " + url);
  Services.prefs.setCharPref(PREF_APP_UPDATE_URL_OVERRIDE, url);
}








function getUpdatesXMLFile(aIsActiveUpdate) {
  var file = getUpdatesRootDir();
  file.append(aIsActiveUpdate ? FILE_UPDATE_ACTIVE : FILE_UPDATES_DB);
  return file;
}











function writeUpdatesToXMLFile(aContent, aIsActiveUpdate) {
  writeFile(getUpdatesXMLFile(aIsActiveUpdate), aContent);
}









function writeStatusFile(aStatus) {
  let file = getUpdatesPatchDir();
  file.append(FILE_UPDATE_STATUS);
  writeFile(file, aStatus + "\n");
}








function writeVersionFile(aVersion) {
  let file = getUpdatesPatchDir();
  file.append(FILE_UPDATE_VERSION);
  writeFile(file, aVersion + "\n");
}






function getUpdatesRootDir() {
  return Services.dirsvc.get(XRE_UPDATE_ROOT_DIR, AUS_Ci.nsIFile);
}






function getUpdatesDir() {
  var dir = getUpdatesRootDir();
  dir.append(DIR_UPDATES);
  return dir;
}






function getUpdatesPatchDir() {
  let dir = getUpdatesDir();
  dir.append(DIR_PATCH);
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







function readStatusFile() {
  let file = getUpdatesPatchDir();
  file.append(FILE_UPDATE_STATUS);

  if (!file.exists()) {
    logTestInfo("update status file does not exists! Path: " + file.path);
    return STATE_NONE;
  }

  return readFile(file).split("\n")[0];
}







function readStatusState() {
  return readStatusFile().split(": ")[0];
}







function readStatusFailedCode() {
  return readStatusFile().split(": ")[1];
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
      throw "Nothing read from input stream!";
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
  } catch (e) {
  }
  return null;
}








function getFileExtension(aFile) {
  return Services.io.newFileURI(aFile).QueryInterface(AUS_Ci.nsIURL).
         fileExtension;
}







function removeUpdateDirsAndFiles() {
  var file = getUpdatesXMLFile(true);
  try {
    if (file.exists())
      file.remove(false);
  } catch (e) {
    dump("Unable to remove file\nPath: " + file.path +
         "\nException: " + e + "\n");
  }

  file = getUpdatesXMLFile(false);
  try {
    if (file.exists())
      file.remove(false);
  } catch (e) {
    dump("Unable to remove file\nPath: " + file.path +
         "\nException: " + e + "\n");
  }

  
  var updatesDir = getUpdatesDir();
  try {
    cleanUpdatesDir(updatesDir);
  } catch (e) {
    dump("Unable to remove files / directories from directory\nPath: " +
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
      if (entry.leafName == DIR_PATCH && entry.parent.leafName == DIR_UPDATES) {
        cleanUpdatesDir(entry);
        entry.permissions = PERMS_DIRECTORY;
      } else {
        try {
          entry.remove(true);
          return;
        } catch (e) {
        }
        cleanUpdatesDir(entry);
        entry.permissions = PERMS_DIRECTORY;
        try {
          entry.remove(true);
        } catch (e) {
          dump("cleanUpdatesDir: unable to remove directory\nPath: " +
               entry.path + "\nException: " + e + "\n");
          throw(e);
        }
      }
    } else {
      entry.permissions = PERMS_FILE;
      try {
        entry.remove(false);
      } catch (e) {
        dump("cleanUpdatesDir: unable to remove file\nPath: " + entry.path +
             "\nException: " + e + "\n");
        throw(e);
      }
    }
  }
}









function removeDirRecursive(aDir) {
  if (!aDir.exists()) {
    return;
  }

  try {
    logTestInfo("attempting to remove directory. Path: " + aDir.path);
    aDir.remove(true);
    return;
  } catch (e) {
    logTestInfo("non-fatal error removing directory. Exception: " + e);
  }

  var dirEntries = aDir.directoryEntries;
  while (dirEntries.hasMoreElements()) {
    var entry = dirEntries.getNext().QueryInterface(AUS_Ci.nsIFile);

    if (entry.isDirectory()) {
      removeDirRecursive(entry);
    } else {
      entry.permissions = PERMS_FILE;
      try {
        logTestInfo("attempting to remove file. Path: " + entry.path);
        entry.remove(false);
      } catch (e) {
        logTestInfo("error removing file. Exception: " + e);
        throw(e);
      }
    }
  }

  aDir.permissions = PERMS_DIRECTORY;
  try {
    logTestInfo("attempting to remove directory. Path: " + aDir.path);
    aDir.remove(true);
  } catch (e) {
    logTestInfo("error removing directory. Exception: " + e);
    throw(e);
  }
}








function getCurrentProcessDir() {
  return Services.dirsvc.get(NS_XPCOM_CURRENT_PROCESS_DIR, AUS_Ci.nsIFile);
}






function getAppBaseDir() {
  return Services.dirsvc.get(XRE_EXECUTABLE_FILE, AUS_Ci.nsIFile).parent;
}









function getGREDir() {
  return Services.dirsvc.get(NS_GRE_DIR, AUS_Ci.nsIFile);
}







function getUpdatedDir() {
  let dir = getAppBaseDir();
#ifdef XP_MACOSX
  dir = dir.parent.parent; 
#endif
  dir.append(DIR_UPDATED);
  logTestInfo("updated directory path: " + dir.path);
  return dir;
}










function logTestInfo(aText, aCaller) {
  let caller = (aCaller ? aCaller : Components.stack.caller);
  let now = new Date;
  let hh = now.getHours();
  let mm = now.getMinutes();
  let ss = now.getSeconds();
  let ms = now.getMilliseconds();
  let time = (hh < 10 ? "0" + hh : hh) + ":" +
             (mm < 10 ? "0" + mm : mm) + ":" +
             (ss < 10 ? "0" + ss : ss) + ":" +
             (ms < 10 ? "00" + ms : ms < 100 ? "0" + ms : ms);
  dump(time + " | TEST-INFO | " + caller.filename + " | [" + caller.name +
       " : " + caller.lineNumber + "] " + aText + "\n");
}










function debugDump(aText, aCaller) {
  if (DEBUG_AUS_TEST) {
    let caller = aCaller ? aCaller : Components.stack.caller;
    logTestInfo(aText, caller);
  }
}
