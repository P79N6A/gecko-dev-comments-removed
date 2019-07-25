#filter substitution








Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");
Components.utils.import("resource://gre/modules/FileUtils.jsm");
Components.utils.import("resource://gre/modules/AddonManager.jsm");
Components.utils.import("resource://gre/modules/Services.jsm");
Components.utils.import("resource://gre/modules/ctypes.jsm");

const Cc = Components.classes;
const Ci = Components.interfaces;
const Cr = Components.results;

const UPDATESERVICE_CID = Components.ID("{B3C290A6-3943-4B89-8BBE-C01EB7B3B311}");
const UPDATESERVICE_CONTRACTID = "@mozilla.org/updates/update-service;1";

const PREF_APP_UPDATE_ALTWINDOWTYPE       = "app.update.altwindowtype";
const PREF_APP_UPDATE_AUTO                = "app.update.auto";
const PREF_APP_UPDATE_BACKGROUND_INTERVAL = "app.update.download.backgroundInterval";
const PREF_APP_UPDATE_BACKGROUNDERRORS    = "app.update.backgroundErrors";
const PREF_APP_UPDATE_BACKGROUNDMAXERRORS = "app.update.backgroundMaxErrors";
const PREF_APP_UPDATE_CERTS_BRANCH        = "app.update.certs.";
const PREF_APP_UPDATE_CERT_CHECKATTRS     = "app.update.cert.checkAttributes";
const PREF_APP_UPDATE_CERT_ERRORS         = "app.update.cert.errors";
const PREF_APP_UPDATE_CERT_MAXERRORS      = "app.update.cert.maxErrors";
const PREF_APP_UPDATE_CERT_REQUIREBUILTIN = "app.update.cert.requireBuiltIn";
const PREF_APP_UPDATE_CHANNEL             = "app.update.channel";
const PREF_APP_UPDATE_ENABLED             = "app.update.enabled";
const PREF_APP_UPDATE_IDLETIME            = "app.update.idletime";
const PREF_APP_UPDATE_INCOMPATIBLE_MODE   = "app.update.incompatible.mode";
const PREF_APP_UPDATE_INTERVAL            = "app.update.interval";
const PREF_APP_UPDATE_LOG                 = "app.update.log";
const PREF_APP_UPDATE_MODE                = "app.update.mode";
const PREF_APP_UPDATE_NEVER_BRANCH        = "app.update.never.";
const PREF_APP_UPDATE_POSTUPDATE          = "app.update.postupdate";
const PREF_APP_UPDATE_PROMPTWAITTIME      = "app.update.promptWaitTime";
const PREF_APP_UPDATE_SHOW_INSTALLED_UI   = "app.update.showInstalledUI";
const PREF_APP_UPDATE_SILENT              = "app.update.silent";
const PREF_APP_UPDATE_STAGE_ENABLED       = "app.update.stage.enabled";
const PREF_APP_UPDATE_URL                 = "app.update.url";
const PREF_APP_UPDATE_URL_DETAILS         = "app.update.url.details";
const PREF_APP_UPDATE_URL_OVERRIDE        = "app.update.url.override";
const PREF_APP_UPDATE_SERVICE_ENABLED     = "app.update.service.enabled";
const PREF_APP_UPDATE_SERVICE_ERRORS      = "app.update.service.errors";
const PREF_APP_UPDATE_SERVICE_MAX_ERRORS  = "app.update.service.maxErrors";

const PREF_PARTNER_BRANCH                 = "app.partner.";
const PREF_APP_DISTRIBUTION               = "distribution.id";
const PREF_APP_DISTRIBUTION_VERSION       = "distribution.version";

const PREF_EM_HOTFIX_ID                   = "extensions.hotfix.id";

const URI_UPDATE_PROMPT_DIALOG  = "chrome://mozapps/content/update/updates.xul";
const URI_UPDATE_HISTORY_DIALOG = "chrome://mozapps/content/update/history.xul";
const URI_BRAND_PROPERTIES      = "chrome://branding/locale/brand.properties";
const URI_UPDATES_PROPERTIES    = "chrome://mozapps/locale/update/updates.properties";
const URI_UPDATE_NS             = "http://www.mozilla.org/2005/app-update";

const CATEGORY_UPDATE_TIMER               = "update-timer";

const KEY_APPDIR          = "XCurProcD";
const KEY_GRED            = "GreD";

#ifdef XP_WIN
#define USE_UPDROOT
#elifdef ANDROID
#define USE_UPDROOT
#endif

#ifdef USE_UPDROOT
const KEY_UPDROOT         = "UpdRootD";
#endif

const DIR_UPDATES         = "updates";
#ifdef XP_MACOSX
const UPDATED_DIR         = "Updated.app";
#else
const UPDATED_DIR         = "updated";
#endif
const FILE_UPDATE_STATUS  = "update.status";
const FILE_UPDATE_VERSION = "update.version";
#ifdef MOZ_WIDGET_ANDROID
const FILE_UPDATE_ARCHIVE = "update.apk";
#else
const FILE_UPDATE_ARCHIVE = "update.mar";
#endif
const FILE_UPDATE_LOG     = "update.log"
const FILE_UPDATES_DB     = "updates.xml";
const FILE_UPDATE_ACTIVE  = "active-update.xml";
const FILE_PERMS_TEST     = "update.test";
const FILE_LAST_LOG       = "last-update.log";
const FILE_BACKUP_LOG     = "backup-update.log";
const FILE_UPDATE_LOCALE  = "update.locale";

const STATE_NONE            = "null";
const STATE_DOWNLOADING     = "downloading";
const STATE_PENDING         = "pending";
const STATE_PENDING_SVC     = "pending-service";
const STATE_APPLYING        = "applying";
const STATE_APPLIED         = "applied";
const STATE_APPLIED_SVC     = "applied-service";
const STATE_SUCCEEDED       = "succeeded";
const STATE_DOWNLOAD_FAILED = "download-failed";
const STATE_FAILED          = "failed";


const WRITE_ERROR        = 7;
const UNEXPECTED_ERROR   = 8;
const ELEVATION_CANCELED = 9;


const SERVICE_UPDATER_COULD_NOT_BE_STARTED = 24;
const SERVICE_NOT_ENOUGH_COMMAND_LINE_ARGS = 25;
const SERVICE_UPDATER_SIGN_ERROR           = 26;
const SERVICE_UPDATER_COMPARE_ERROR        = 27;
const SERVICE_UPDATER_IDENTITY_ERROR       = 28;
const SERVICE_STILL_APPLYING_ON_SUCCESS    = 29;
const SERVICE_STILL_APPLYING_ON_FAILURE    = 30;
const SERVICE_UPDATER_NOT_FIXED_DRIVE      = 31;
const SERVICE_COULD_NOT_LOCK_UPDATER       = 32;
const SERVICE_INSTALLDIR_ERROR             = 33;

const CERT_ATTR_CHECK_FAILED_NO_UPDATE  = 100;
const CERT_ATTR_CHECK_FAILED_HAS_UPDATE = 101;
const BACKGROUNDCHECK_MULTIPLE_FAILURES = 110;

const DOWNLOAD_CHUNK_SIZE           = 300000; 
const DOWNLOAD_BACKGROUND_INTERVAL  = 600;    
const DOWNLOAD_FOREGROUND_INTERVAL  = 0;

const UPDATE_WINDOW_NAME      = "Update:Wizard";



const DEFAULT_SERVICE_MAX_ERRORS = 10;

var gLocale     = null;

XPCOMUtils.defineLazyGetter(this, "gLogEnabled", function aus_gLogEnabled() {
  return getPref("getBoolPref", PREF_APP_UPDATE_LOG, false);
});

XPCOMUtils.defineLazyGetter(this, "gUpdateBundle", function aus_gUpdateBundle() {
  return Services.strings.createBundle(URI_UPDATES_PROPERTIES);
});


XPCOMUtils.defineLazyGetter(this, "gCertUtils", function aus_gCertUtils() {
  let temp = { };
  Components.utils.import("resource://gre/modules/CertUtils.jsm", temp);
  return temp;
});

XPCOMUtils.defineLazyGetter(this, "gABI", function aus_gABI() {
  let abi = null;
  try {
    abi = Services.appinfo.XPCOMABI;
  }
  catch (e) {
    LOG("gABI - XPCOM ABI unknown: updates are not possible.");
  }
#ifdef XP_MACOSX
  
  
  let macutils = Cc["@mozilla.org/xpcom/mac-utils;1"].
                 getService(Ci.nsIMacUtils);

  if (macutils.isUniversalBinary)
    abi += "-u-" + macutils.architecturesInBinary;
#ifdef MOZ_SHARK
  
  abi += "-shark"
#endif
#endif
  return abi;
});

XPCOMUtils.defineLazyGetter(this, "gOSVersion", function aus_gOSVersion() {
  let osVersion;
  let sysInfo = Cc["@mozilla.org/system-info;1"].
                getService(Ci.nsIPropertyBag2);
  try {
    osVersion = sysInfo.getProperty("name") + " " + sysInfo.getProperty("version");
  }
  catch (e) {
    LOG("gOSVersion - OS Version unknown: updates are not possible.");
  }

  if (osVersion) {
#ifdef XP_WIN
    const BYTE = ctypes.uint8_t;
    const WORD = ctypes.uint16_t;
    const DWORD = ctypes.uint32_t;
    const WCHAR = ctypes.jschar;
    const BOOL = ctypes.int;

    
    
    const SZCSDVERSIONLENGTH = 128;
    const OSVERSIONINFOEXW = new ctypes.StructType('OSVERSIONINFOEXW',
        [
        {dwOSVersionInfoSize: DWORD},
        {dwMajorVersion: DWORD},
        {dwMinorVersion: DWORD},
        {dwBuildNumber: DWORD},
        {dwPlatformId: DWORD},
        {szCSDVersion: ctypes.ArrayType(WCHAR, SZCSDVERSIONLENGTH)},
        {wServicePackMajor: WORD},
        {wServicePackMinor: WORD},
        {wSuiteMask: WORD},
        {wProductType: BYTE},
        {wReserved: BYTE}
        ]);

    
    
    const SYSTEM_INFO = new ctypes.StructType('SYSTEM_INFO',
        [
        {wProcessorArchitecture: WORD},
        {wReserved: WORD},
        {dwPageSize: DWORD},
        {lpMinimumApplicationAddress: ctypes.voidptr_t},
        {lpMaximumApplicationAddress: ctypes.voidptr_t},
        {dwActiveProcessorMask: DWORD.ptr},
        {dwNumberOfProcessors: DWORD},
        {dwProcessorType: DWORD},
        {dwAllocationGranularity: DWORD},
        {wProcessorLevel: WORD},
        {wProcessorRevision: WORD}
        ]);

    let kernel32 = false;
    try {
      kernel32 = ctypes.open("Kernel32");
    } catch (e) {
      LOG("gOSVersion - Unable to open kernel32! " + e);
      osVersion += ".unknown (unknown)";
    }

    if(kernel32) {
      try {
        
        try {
          let GetVersionEx = kernel32.declare("GetVersionExW",
                                              ctypes.default_abi,
                                              BOOL,
                                              OSVERSIONINFOEXW.ptr);
          let winVer = OSVERSIONINFOEXW();
          winVer.dwOSVersionInfoSize = OSVERSIONINFOEXW.size;

          if(0 !== GetVersionEx(winVer.address())) {
            osVersion += "." + winVer.wServicePackMajor
                      +  "." + winVer.wServicePackMinor;
          } else {
            LOG("gOSVersion - Unknown failure in GetVersionEX (returned 0)");
            osVersion += ".unknown";
          }
        } catch (e) {
          LOG("gOSVersion - error getting service pack information. Exception: " + e);
          osVersion += ".unknown";
        }

        
        let arch = "unknown";
        try {
          let GetNativeSystemInfo = kernel32.declare("GetNativeSystemInfo",
                                                     ctypes.default_abi,
                                                     ctypes.void_t,
                                                     SYSTEM_INFO.ptr);
          let sysInfo = SYSTEM_INFO();
          
          sysInfo.wProcessorArchitecture = 0xffff;

          GetNativeSystemInfo(sysInfo.address());
          switch(sysInfo.wProcessorArchitecture) {
            case 9:
              arch = "x64";
              break;
            case 6:
              arch = "IA64";
              break;
            case 0:
              arch = "x86";
              break;
          }
        } catch (e) {
          LOG("gOSVersion - error getting processor architecture.  Exception: " + e);
        } finally {
          osVersion += " (" + arch + ")";
        }
      } finally {
        kernel32.close();
      }
    }
#endif

    try {
      osVersion += " (" + sysInfo.getProperty("secondaryLibrary") + ")";
    }
    catch (e) {
      
    }
    osVersion = encodeURIComponent(osVersion);
  }
  return osVersion;
});








function testWriteAccess(updateTestFile, createDirectory) {
  const NORMAL_FILE_TYPE = Ci.nsILocalFile.NORMAL_FILE_TYPE;
  const DIRECTORY_TYPE = Ci.nsILocalFile.DIRECTORY_TYPE;
  if (updateTestFile.exists())
    updateTestFile.remove(false);
  updateTestFile.create(createDirectory ? DIRECTORY_TYPE : NORMAL_FILE_TYPE,
                        createDirectory ? FileUtils.PERMS_DIRECTORY : FileUtils.PERMS_FILE);
  updateTestFile.remove(false);
}

XPCOMUtils.defineLazyGetter(this, "gCanApplyUpdates", function aus_gCanApplyUpdates() {
  try {
    var updateTestFile = getUpdateFile([FILE_PERMS_TEST]);
    LOG("gCanApplyUpdates - testing write access " + updateTestFile.path);
    testWriteAccess(updateTestFile, false);
#ifdef XP_WIN
    var sysInfo = Cc["@mozilla.org/system-info;1"].
                  getService(Ci.nsIPropertyBag2);

    
    var windowsVersion = sysInfo.getProperty("version");
    LOG("gCanApplyUpdates - windowsVersion = " + windowsVersion);

  










    var userCanElevate = false;

    if (parseFloat(windowsVersion) >= 6) {
      try {
        var fileLocator = Cc["@mozilla.org/file/directory_service;1"].
                          getService(Ci.nsIProperties);
        
        
        var dir = fileLocator.get(KEY_UPDROOT, Ci.nsIFile);
        
        userCanElevate = Services.appinfo.QueryInterface(Ci.nsIWinAppHelper).
                         userCanElevate;
        LOG("gCanApplyUpdates - on Vista, userCanElevate: " + userCanElevate);
      }
      catch (ex) {
        
        
        
        LOG("gCanApplyUpdates - on Vista, appDir is not under Program Files");
      }
    }

    



















    if (!userCanElevate) {
      
      var appDirTestFile = FileUtils.getFile(KEY_APPDIR, [FILE_PERMS_TEST]);
      LOG("gCanApplyUpdates - testing write access " + appDirTestFile.path);
      if (appDirTestFile.exists())
        appDirTestFile.remove(false)
      appDirTestFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
      appDirTestFile.remove(false);
    }
#endif 
  }
  catch (e) {
     LOG("gCanApplyUpdates - unable to apply updates. Exception: " + e);
    
    return false;
  }

  LOG("gCanApplyUpdates - able to apply updates");
  return true;
});

XPCOMUtils.defineLazyGetter(this, "gCanStageUpdates", function aus_gCanStageUpdates() {
  
  if (!getPref("getBoolPref", PREF_APP_UPDATE_STAGE_ENABLED, false)) {
    LOG("gCanStageUpdates - staging updates is disabled by preference " + PREF_APP_UPDATE_STAGE_ENABLED);
    return false;
  }

#ifdef XP_WIN
  if (getPref("getBoolPref", PREF_APP_UPDATE_SERVICE_ENABLED, false)) {
    
    
    LOG("gCanStageUpdates - able to stage updates because we'll use the service");
    return true;
  }
#endif

  try {
    var updateTestFile = getInstallDirRoot();
    updateTestFile.append(FILE_PERMS_TEST);
    LOG("gCanStageUpdates - testing write access " + updateTestFile.path);
    testWriteAccess(updateTestFile, true);
#ifndef XP_MACOSX
    
    
    
    updateTestFile = getInstallDirRoot().parent;
    updateTestFile.append(FILE_PERMS_TEST);
    LOG("gCanStageUpdates - testing write access " + updateTestFile.path);
    updateTestFile.createUnique(Ci.nsILocalFile.DIRECTORY_TYPE,
                                FileUtils.PERMS_DIRECTORY);
    updateTestFile.remove(false);
#endif
  }
  catch (e) {
     LOG("gCanStageUpdates - unable to stage updates. Exception: " + e);
    
    return false;
  }

  LOG("gCanStageUpdates - able to stage updates");
  return true;
});

XPCOMUtils.defineLazyGetter(this, "gCanCheckForUpdates", function aus_gCanCheckForUpdates() {
  
  
  
  var enabled = getPref("getBoolPref", PREF_APP_UPDATE_ENABLED, true);
  if (!enabled && Services.prefs.prefIsLocked(PREF_APP_UPDATE_ENABLED)) {
    LOG("gCanCheckForUpdates - unable to automatically check for updates, " +
        "disabled by pref");
    return false;
  }

  
  if (!gABI) {
    LOG("gCanCheckForUpdates - unable to check for updates, unknown ABI");
    return false;
  }

  
  if (!gOSVersion) {
    LOG("gCanCheckForUpdates - unable to check for updates, unknown OS " +
        "version");
    return false;
  }

  LOG("gCanCheckForUpdates - able to check for updates");
  return true;
});






function LOG(string) {
  if (gLogEnabled) {
    dump("*** AUS:SVC " + string + "\n");
    Services.console.logStringMessage("AUS:SVC " + string);
  }
}













function getPref(func, preference, defaultValue) {
  try {
    return Services.prefs[func](preference);
  }
  catch (e) {
  }
  return defaultValue;
}




function binaryToHex(input) {
  var result = "";
  for (var i = 0; i < input.length; ++i) {
    var hex = input.charCodeAt(i).toString(16);
    if (hex.length == 1)
      hex = "0" + hex;
    result += hex;
  }
  return result;
}









function getUpdateDirCreate(pathArray) {
#ifdef USE_UPDROOT
  try {
    let dir = FileUtils.getDir(KEY_UPDROOT, pathArray, true);
    return dir;
  } catch (e) {
  }
#endif
  return FileUtils.getDir(KEY_APPDIR, pathArray, true);
}








function getInstallDirRoot() {
  var dir = FileUtils.getDir(KEY_APPDIR, [], false);
#ifdef XP_MACOSX
  
  dir = dir.parent.parent;
#endif
  return dir;
}











function getUpdateFile(pathArray) {
  var file = getUpdateDirCreate(pathArray.slice(0, -1));
  file.append(pathArray[pathArray.length - 1]);
  return file;
}











function getStatusTextFromCode(code, defaultCode) {
  var reason;
  try {
    reason = gUpdateBundle.GetStringFromName("check_error-" + code);
    LOG("getStatusTextFromCode - transfer error: " + reason + ", code: " +
        code);
  }
  catch (e) {
    
    reason = gUpdateBundle.GetStringFromName("check_error-" + defaultCode);
    LOG("getStatusTextFromCode - transfer error: " + reason +
        ", default code: " + defaultCode);
  }
  return reason;
}





function getUpdatesDir() {
  
  
  return getUpdateDirCreate([DIR_UPDATES, "0"]);
}

#ifndef USE_UPDROOT






function getUpdatesDirInApplyToDir() {
  var dir = FileUtils.getDir(KEY_APPDIR, []);
#ifdef XP_MACOSX
  dir = dir.parent.parent; 
#endif
  dir.append(UPDATED_DIR);
#ifdef XP_MACOSX
  dir.append("Contents");
  dir.append("MacOS");
#endif
  dir.append(DIR_UPDATES);
  if (!dir.exists()) {
    dir.create(Ci.nsILocalFile.DIRECTORY_TYPE, 0755);
  }
  return dir;
}
#endif








function readStatusFile(dir) {
  var statusFile = dir.clone();
  statusFile.append(FILE_UPDATE_STATUS);
  var status = readStringFromFile(statusFile) || STATE_NONE;
  LOG("readStatusFile - status: " + status + ", path: " + statusFile.path);
  return status;
}











function writeStatusFile(dir, state) {
  var statusFile = dir.clone();
  statusFile.append(FILE_UPDATE_STATUS);
  writeStringToFile(statusFile, state);
}








function shouldUseService() {
#ifdef MOZ_MAINTENANCE_SERVICE
  return getPref("getBoolPref", 
                 PREF_APP_UPDATE_SERVICE_ENABLED, false);
#else
  return false;
#endif
}
















function writeVersionFile(dir, version) {
  var versionFile = dir.clone();
  versionFile.append(FILE_UPDATE_VERSION);
  writeStringToFile(versionFile, version);
}









function cleanUpUpdatesDir(aBackgroundUpdate) {
  
  try {
    var updateDir = getUpdatesDir();
  }
  catch (e) {
    return;
  }

  var e = updateDir.directoryEntries;
  while (e.hasMoreElements()) {
    var f = e.getNext().QueryInterface(Ci.nsIFile);
    
    if (f.leafName == FILE_UPDATE_LOG) {
      var dir;
      try {
#ifdef USE_UPDROOT
        
        
        
        
        
        
        dir = f.parent.parent;
#else
        
        
        
        
        
        if (aBackgroundUpdate) {
          dir = getUpdatesDirInApplyToDir();
        } else {
          dir = f.parent.parent;
        }
#endif
        var logFile = dir.clone();
        logFile.append(FILE_LAST_LOG);
        if (logFile.exists()) {
          try {
            logFile.moveTo(dir, FILE_BACKUP_LOG);
          }
          catch (e) {
            LOG("cleanUpUpdatesDir - failed to rename file " + logFile.path +
                " to " + FILE_BACKUP_LOG);
          }
        }
        f.moveTo(dir, FILE_LAST_LOG);
        if (aBackgroundUpdate) {
          
          
          break;
        } else {
          continue;
        }
      }
      catch (e) {
        LOG("cleanUpUpdatesDir - failed to move file " + f.path + " to " +
            dir.path + " and rename it to " + FILE_LAST_LOG);
      }
    } else if (aBackgroundUpdate) {
      
      
      
      continue;
    }
    
    
    
    try {
      f.remove(true);
    }
    catch (e) {
      LOG("cleanUpUpdatesDir - failed to remove file " + f.path);
    }
  }
}




function cleanupActiveUpdate() {
  
  var um = Cc["@mozilla.org/updates/update-manager;1"].
           getService(Ci.nsIUpdateManager);
  um.activeUpdate = null;
  um.saveUpdates();

  
  cleanUpUpdatesDir();
}







function getLocale() {
  if (gLocale)
    return gLocale;

  for each (res in ['app', 'gre']) {
    var channel = Services.io.newChannel("resource://" + res + "/" + FILE_UPDATE_LOCALE, null, null);
    try {
      var inputStream = channel.open();
      gLocale = readStringFromInputStream(inputStream);
    } catch(e) {}
    if (gLocale)
      break;
  }

  if (!gLocale)
    throw Components.Exception(FILE_UPDATE_LOCALE + " file doesn't exist in " +
                               "either the " + KEY_APPDIR + " or " + KEY_GRED +
                               " directories", Cr.NS_ERROR_FILE_NOT_FOUND);

  LOG("getLocale - getting locale from file: " + channel.originalURI.spec +
      ", locale: " + gLocale);
  return gLocale;
}






function getUpdateChannel() {
  
  
  var channel = "@MOZ_UPDATE_CHANNEL@";
  var prefName;
  var prefValue;

  try {
    channel = Services.prefs.getDefaultBranch(null).
              getCharPref(PREF_APP_UPDATE_CHANNEL);
  } catch (e) {
    
  }

  try {
    var partners = Services.prefs.getChildList(PREF_PARTNER_BRANCH);
    if (partners.length) {
      channel += "-cck";
      partners.sort();

      for each (prefName in partners) {
        prefValue = Services.prefs.getCharPref(prefName);
        channel += "-" + prefValue;
      }
    }
  }
  catch (e) {
    Components.utils.reportError(e);
  }

  return channel;
}


function getDistributionPrefValue(aPrefName) {
  var prefValue = "default";

  try {
    prefValue = Services.prefs.getDefaultBranch(null).getCharPref(aPrefName);
  } catch (e) {
    
  }

  return prefValue;
}





function ArrayEnumerator(aItems) {
  this._index = 0;
  if (aItems) {
    for (var i = 0; i < aItems.length; ++i) {
      if (!aItems[i])
        aItems.splice(i, 1);
    }
  }
  this._contents = aItems;
}

ArrayEnumerator.prototype = {
  _index: 0,
  _contents: [],

  hasMoreElements: function ArrayEnumerator_hasMoreElements() {
    return this._index < this._contents.length;
  },

  getNext: function ArrayEnumerator_getNext() {
    return this._contents[this._index++];
  }
};





function writeStringToFile(file, text) {
  var fos = FileUtils.openSafeFileOutputStream(file)
  text += "\n";
  fos.write(text, text.length);
  FileUtils.closeSafeFileOutputStream(fos);
}

function readStringFromInputStream(inputStream) {
  var sis = Cc["@mozilla.org/scriptableinputstream;1"].
            createInstance(Ci.nsIScriptableInputStream);
  sis.init(inputStream);
  var text = sis.read(sis.available());
  sis.close();
  if (text[text.length - 1] == "\n")
    text = text.slice(0, -1);
  return text;
}





function readStringFromFile(file) {
  if (!file.exists()) {
    LOG("readStringFromFile - file doesn't exist: " + file.path);
    return null;
  }
  var fis = Cc["@mozilla.org/network/file-input-stream;1"].
            createInstance(Ci.nsIFileInputStream);
  fis.init(file, FileUtils.MODE_RDONLY, FileUtils.PERMS_FILE, 0);
  return readStringFromInputStream(fis);
}

function handleUpdateFailure(update, errorCode) {
  update.errorCode = parseInt(errorCode);
  if (update.errorCode == WRITE_ERROR) {
    Cc["@mozilla.org/updates/update-prompt;1"].
      createInstance(Ci.nsIUpdatePrompt).
      showUpdateError(update);
    writeStatusFile(getUpdatesDir(), update.state = STATE_PENDING);
    return true;
  }

  if (update.errorCode == ELEVATION_CANCELED) {
    writeStatusFile(getUpdatesDir(), update.state = STATE_PENDING);
    return true;
  }

  if (update.errorCode == SERVICE_UPDATER_COULD_NOT_BE_STARTED ||
      update.errorCode == SERVICE_NOT_ENOUGH_COMMAND_LINE_ARGS ||
      update.errorCode == SERVICE_UPDATER_SIGN_ERROR ||
      update.errorCode == SERVICE_UPDATER_COMPARE_ERROR ||
      update.errorCode == SERVICE_UPDATER_IDENTITY_ERROR ||
      update.errorCode == SERVICE_STILL_APPLYING_ON_SUCCESS ||
      update.errorCode == SERVICE_STILL_APPLYING_ON_FAILURE ||
      update.errorCode == SERVICE_UPDATER_NOT_FIXED_DRIVE ||
      update.errorCode == SERVICE_COULD_NOT_LOCK_UPDATER ||
      update.errorCode == SERVICE_INSTALLDIR_ERROR) {

    var failCount = getPref("getIntPref", 
                            PREF_APP_UPDATE_SERVICE_ERRORS, 0);
    var maxFail = getPref("getIntPref", 
                          PREF_APP_UPDATE_SERVICE_MAX_ERRORS, 
                          DEFAULT_SERVICE_MAX_ERRORS);

    
    
    
    if (failCount >= maxFail) {
      Services.prefs.setBoolPref(PREF_APP_UPDATE_SERVICE_ENABLED, false);
      Services.prefs.clearUserPref(PREF_APP_UPDATE_SERVICE_ERRORS);
    } else {
      failCount++;
      Services.prefs.setIntPref(PREF_APP_UPDATE_SERVICE_ERRORS, 
                                failCount);
    }

    writeStatusFile(getUpdatesDir(), update.state = STATE_PENDING);
    return true;
  }
  
  return false;
}







function handleFallbackToCompleteUpdate(update, postStaging) {
  cleanupActiveUpdate();

  update.statusText = gUpdateBundle.GetStringFromName("patchApplyFailure");
  var oldType = update.selectedPatch ? update.selectedPatch.type
                                     : "complete";
  if (update.selectedPatch && oldType == "partial" && update.patchCount == 2) {
    
    
    LOG("UpdateService:_postUpdateProcessing - install of partial patch " +
        "failed, downloading complete patch");
    var status = Cc["@mozilla.org/updates/update-service;1"].
                 getService(Ci.nsIApplicationUpdateService).
                 downloadUpdate(update, !postStaging);
    if (status == STATE_NONE)
      cleanupActiveUpdate();
  }
  else {
    LOG("handleFallbackToCompleteUpdate - install of complete or " +
        "only one patch offered failed.");
  }
  update.QueryInterface(Ci.nsIWritablePropertyBag);
  update.setProperty("patchingFailed", oldType);
}








function UpdatePatch(patch) {
  this._properties = {};
  for (var i = 0; i < patch.attributes.length; ++i) {
    var attr = patch.attributes.item(i);
    attr.QueryInterface(Ci.nsIDOMAttr);
    switch (attr.name) {
    case "selected":
      this.selected = attr.value == "true";
      break;
    case "size":
      if (0 == parseInt(attr.value)) {
        LOG("UpdatePatch:init - 0-sized patch!");
        throw Cr.NS_ERROR_ILLEGAL_VALUE;
      }
      
    default:
      this[attr.name] = attr.value;
      break;
    };
  }
}
UpdatePatch.prototype = {
  


  serialize: function UpdatePatch_serialize(updates) {
    var patch = updates.createElementNS(URI_UPDATE_NS, "patch");
    patch.setAttribute("type", this.type);
    patch.setAttribute("URL", this.URL);
    
    if (this.finalURL)
      patch.setAttribute("finalURL", this.finalURL);
    patch.setAttribute("hashFunction", this.hashFunction);
    patch.setAttribute("hashValue", this.hashValue);
    patch.setAttribute("size", this.size);
    patch.setAttribute("selected", this.selected);
    patch.setAttribute("state", this.state);

    for (var p in this._properties) {
      if (this._properties[p].present)
        patch.setAttribute(p, this._properties[p].data);
    }

    return patch;
  },

  


  _properties: null,

  


  setProperty: function UpdatePatch_setProperty(name, value) {
    this._properties[name] = { data: value, present: true };
  },

  


  deleteProperty: function UpdatePatch_deleteProperty(name) {
    if (name in this._properties)
      this._properties[name].present = false;
    else
      throw Cr.NS_ERROR_FAILURE;
  },

  


  get enumerator() {
    var properties = [];
    for (var p in this._properties)
      properties.push(this._properties[p].data);
    return new ArrayEnumerator(properties);
  },

  


  getProperty: function UpdatePatch_getProperty(name) {
    if (name in this._properties &&
        this._properties[name].present)
      return this._properties[name].data;
    throw Cr.NS_ERROR_FAILURE;
  },

  



  get statusFileExists() {
    var statusFile = getUpdatesDir();
    statusFile.append(FILE_UPDATE_STATUS);
    return statusFile.exists();
  },

  


  get state() {
    if (this._properties.state)
      return this._properties.state;
    return STATE_NONE;
  },
  set state(val) {
    this._properties.state = val;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdatePatch,
                                         Ci.nsIPropertyBag,
                                         Ci.nsIWritablePropertyBag])
};









function Update(update) {
  this._properties = {};
  this._patches = [];
  this.isCompleteUpdate = false;
  this.showPrompt = false;
  this.showSurvey = false;
  this.showNeverForVersion = false;
  this.channel = "default"

  
  
  if (!update)
    return;

  const ELEMENT_NODE = Ci.nsIDOMNode.ELEMENT_NODE;
  for (var i = 0; i < update.childNodes.length; ++i) {
    var patchElement = update.childNodes.item(i);
    if (patchElement.nodeType != ELEMENT_NODE ||
        patchElement.localName != "patch")
      continue;

    patchElement.QueryInterface(Ci.nsIDOMElement);
    try {
      var patch = new UpdatePatch(patchElement);
    } catch (e) {
      continue;
    }
    this._patches.push(patch);
  }

  if (0 == this._patches.length)
    throw Cr.NS_ERROR_ILLEGAL_VALUE;

  
  
  if (!update.hasAttribute("appVersion")) {
    if (update.getAttribute("type") == "major") {
      if (update.hasAttribute("detailsURL")) {
        this.billboardURL = update.getAttribute("detailsURL");
        this.showPrompt = true;
        this.showNeverForVersion = true;
      }
    }
  }

  for (var i = 0; i < update.attributes.length; ++i) {
    var attr = update.attributes.item(i);
    attr.QueryInterface(Ci.nsIDOMAttr);
    if (attr.value == "undefined")
      continue;
    else if (attr.name == "detailsURL")
      this._detailsURL = attr.value;
    else if (attr.name == "extensionVersion") {
      
      
      if (!this.appVersion)
        this.appVersion = attr.value;
    }
    else if (attr.name == "installDate" && attr.value)
      this.installDate = parseInt(attr.value);
    else if (attr.name == "isCompleteUpdate")
      this.isCompleteUpdate = attr.value == "true";
    else if (attr.name == "isSecurityUpdate")
      this.isSecurityUpdate = attr.value == "true";
    else if (attr.name == "showNeverForVersion")
      this.showNeverForVersion = attr.value == "true";
    else if (attr.name == "showPrompt")
      this.showPrompt = attr.value == "true";
    else if (attr.name == "showSurvey")
      this.showSurvey = attr.value == "true";
    else if (attr.name == "version") {
      
      
      if (!this.displayVersion)
        this.displayVersion = attr.value;
    }
    else {
      this[attr.name] = attr.value;

      switch (attr.name) {
      case "appVersion":
      case "billboardURL":
      case "buildID":
      case "channel":
      case "displayVersion":
      case "licenseURL":
      case "name":
      case "platformVersion":
      case "previousAppVersion":
      case "serviceURL":
      case "statusText":
      case "type":
        break;
      default:
        
        
        
        this.setProperty(attr.name, attr.value);
        break;
      };
    }
  }

  
  
  if (!this.installDate && this.installDate != 0)
    this.installDate = (new Date()).getTime();

  
  
  var name = "";
  if (update.hasAttribute("name"))
    name = update.getAttribute("name");
  else {
    var brandBundle = Services.strings.createBundle(URI_BRAND_PROPERTIES);
    var appName = brandBundle.GetStringFromName("brandShortName");
    name = gUpdateBundle.formatStringFromName("updateName",
                                              [appName, this.displayVersion], 2);
  }
  this.name = name;
}
Update.prototype = {
  


  get patchCount() {
    return this._patches.length;
  },

  


  getPatchAt: function Update_getPatchAt(index) {
    return this._patches[index];
  },

  







  _state: "",
  set state(state) {
    if (this.selectedPatch)
      this.selectedPatch.state = state;
    this._state = state;
    return state;
  },
  get state() {
    if (this.selectedPatch)
      return this.selectedPatch.state;
    return this._state;
  },

  


  errorCode: 0,

  


  get selectedPatch() {
    for (var i = 0; i < this.patchCount; ++i) {
      if (this._patches[i].selected)
        return this._patches[i];
    }
    return null;
  },

  


  get detailsURL() {
    if (!this._detailsURL) {
      try {
        
        
        return Services.urlFormatter.formatURLPref(PREF_APP_UPDATE_URL_DETAILS);
      }
      catch (e) {
      }
    }
    return this._detailsURL || "";
  },

  


  serialize: function Update_serialize(updates) {
    var update = updates.createElementNS(URI_UPDATE_NS, "update");
    update.setAttribute("appVersion", this.appVersion);
    update.setAttribute("buildID", this.buildID);
    update.setAttribute("channel", this.channel);
    update.setAttribute("displayVersion", this.displayVersion);
    
    update.setAttribute("extensionVersion", this.appVersion);
    update.setAttribute("installDate", this.installDate);
    update.setAttribute("isCompleteUpdate", this.isCompleteUpdate);
    update.setAttribute("name", this.name);
    update.setAttribute("serviceURL", this.serviceURL);
    update.setAttribute("showNeverForVersion", this.showNeverForVersion);
    update.setAttribute("showPrompt", this.showPrompt);
    update.setAttribute("showSurvey", this.showSurvey);
    update.setAttribute("type", this.type);
    
    update.setAttribute("version", this.displayVersion);

    
    if (this.billboardURL)
      update.setAttribute("billboardURL", this.billboardURL);
    if (this.detailsURL)
      update.setAttribute("detailsURL", this.detailsURL);
    if (this.licenseURL)
      update.setAttribute("licenseURL", this.licenseURL);
    if (this.platformVersion)
      update.setAttribute("platformVersion", this.platformVersion);
    if (this.previousAppVersion)
      update.setAttribute("previousAppVersion", this.previousAppVersion);
    if (this.statusText)
      update.setAttribute("statusText", this.statusText);
    updates.documentElement.appendChild(update);

    for (var p in this._properties) {
      if (this._properties[p].present)
        update.setAttribute(p, this._properties[p].data);
    }

    for (var i = 0; i < this.patchCount; ++i)
      update.appendChild(this.getPatchAt(i).serialize(updates));

    return update;
  },

  


  _properties: null,

  


  setProperty: function Update_setProperty(name, value) {
    this._properties[name] = { data: value, present: true };
  },

  


  deleteProperty: function Update_deleteProperty(name) {
    if (name in this._properties)
      this._properties[name].present = false;
    else
      throw Cr.NS_ERROR_FAILURE;
  },

  


  get enumerator() {
    var properties = [];
    for (var p in this._properties)
      properties.push(this._properties[p].data);
    return new ArrayEnumerator(properties);
  },

  


  getProperty: function Update_getProperty(name) {
    if (name in this._properties && this._properties[name].present)
      return this._properties[name].data;
    throw Cr.NS_ERROR_FAILURE;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdate,
                                         Ci.nsIPropertyBag,
                                         Ci.nsIWritablePropertyBag])
};

const UpdateServiceFactory = {
  _instance: null,
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return this._instance == null ? this._instance = new UpdateService() :
                                    this._instance;
  }
};






function UpdateService() {
  Services.obs.addObserver(this, "xpcom-shutdown", false);
}

UpdateService.prototype = {
  



  _downloader: null,

  


  _incompatAddonsCount: 0,

  








  observe: function AUS_observe(subject, topic, data) {
    switch (topic) {
    case "post-update-processing":
      
      this._postUpdateProcessing();
      break;
    case "xpcom-shutdown":
      Services.obs.removeObserver(this, "xpcom-shutdown");

      this.pauseDownload();
      
      this._downloader = null;
      break;
    }
  },

  








  





  _postUpdateProcessing: function AUS__postUpdateProcessing() {
    if (!this.canCheckForUpdates || !this.canApplyUpdates) {
      LOG("UpdateService:_postUpdateProcessing - unable to check for or apply " +
          "updates... returning early");
      return;
    }

    var status = readStatusFile(getUpdatesDir());
    
    
    if (status == STATE_NONE) {
      LOG("UpdateService:_postUpdateProcessing - no status, no update");
      cleanupActiveUpdate();
      return;
    }

    var um = Cc["@mozilla.org/updates/update-manager;1"].
             getService(Ci.nsIUpdateManager);
    var update = um.activeUpdate;

    if (status == STATE_DOWNLOADING) {
      LOG("UpdateService:_postUpdateProcessing - patch found in downloading " +
          "state");
      if (update && update.state != STATE_SUCCEEDED) {
        
        var status = this.downloadUpdate(update, true);
        if (status == STATE_NONE)
          cleanupActiveUpdate();
      }
      return;
    }

    if (status == STATE_APPLYING) {
      
      
      
      
      
      
      
      
      
      
      
      
      
      if (update &&
          (update.state == STATE_PENDING || update.state == STATE_PENDING_SVC)) {
        LOG("UpdateService:_postUpdateProcessing - patch found in applying " +
            "state for the first time");
        update.state = STATE_APPLYING;
        um.saveUpdates();
      } else { 
        LOG("UpdateService:_postUpdateProcessing - patch found in applying " +
            "state for the second time");
        cleanupActiveUpdate();
      }
      return;
    }

    if (!update)
      update = new Update(null);

    var prompter = Cc["@mozilla.org/updates/update-prompt;1"].
                   createInstance(Ci.nsIUpdatePrompt);

    update.state = status;
    this._sendStatusCodeTelemetryPing(status);

    if (status == STATE_SUCCEEDED) {
      
      
      
      
      
      this._sendBoolPrefTelemetryPing(PREF_APP_UPDATE_ENABLED,
                                      "UPDATER_UPDATES_ENABLED");
      this._sendBoolPrefTelemetryPing(PREF_APP_UPDATE_AUTO,
                                      "UPDATER_UPDATES_AUTOMATIC");
      this._sendBoolPrefTelemetryPing(PREF_APP_UPDATE_STAGE_ENABLED,
                                      "UPDATER_STAGE_ENABLED");

#ifdef XP_WIN
      this._sendBoolPrefTelemetryPing(PREF_APP_UPDATE_SERVICE_ENABLED,
                                      "UPDATER_SERVICE_ENABLED");
      this._sendIntPrefTelemetryPing(PREF_APP_UPDATE_SERVICE_ERRORS,
                                     "UPDATER_SERVICE_ERRORS");
      this._sendServiceInstalledTelemetryPing();
#endif

      update.statusText = gUpdateBundle.GetStringFromName("installSuccess");

      
      um.activeUpdate = update;
      Services.prefs.setBoolPref(PREF_APP_UPDATE_POSTUPDATE, true);
      prompter.showUpdateInstalled();

      
      cleanupActiveUpdate();
    }
    else {
      
      
      
      
      
      
      
      
      var ary = status.split(":");
      update.state = ary[0];
      if (update.state == STATE_FAILED && ary[1]) {
        if (handleUpdateFailure(update, ary[1])) {
          return;
        }
      }

      
      handleFallbackToCompleteUpdate(update, false);

      prompter.showUpdateError(update);
    }
  },

  







  _sendBoolPrefTelemetryPing: function AUS__boolTelemetryPing(pref, histogram) {
    try {
      
      
      
      let val = getPref("getBoolPref", pref, false);
      Services.telemetry.getHistogramById(histogram).add(+val);
    } catch(e) {
      
      Components.utils.reportError(e);
    }
  },

#ifdef XP_WIN
  



  _sendServiceInstalledTelemetryPing: function AUS__svcInstallTelemetryPing() {
    let installed = 0;
    try {
      let wrk = Components.classes["@mozilla.org/windows-registry-key;1"]
                .createInstance(Components.interfaces.nsIWindowsRegKey);
      wrk.open(wrk.ROOT_KEY_LOCAL_MACHINE,
               "SOFTWARE\\Mozilla\\MaintenanceService",
               wrk.ACCESS_READ | wrk.WOW64_64);
      installed = wrk.readIntValue("Installed");
      wrk.close();
    } catch(e) {
    }
    try {
      let h = Services.telemetry.getHistogramById("UPDATER_SERVICE_INSTALLED");
      h.add(installed);
    } catch(e) {
      
      Components.utils.reportError(e);
    }
  },
#endif

  







  _sendIntPrefTelemetryPing: function AUS__intTelemetryPing(pref, histogram) {
    try {
      
      
      
      let val = getPref("getIntPref", pref, 0);
      Services.telemetry.getHistogramById(histogram).add(val);
    } catch(e) {
      
      Components.utils.reportError(e);
    }
  },


  





  _sendStatusCodeTelemetryPing: function AUS__statusTelemetryPing(status) {
    try {
      let parts = status.split(":");
      if ((parts.length == 1 && status != STATE_SUCCEEDED) ||
          (parts.length > 1  && parts[0] != STATE_FAILED)) {
        
        return;
      }
      let result = 0; 
      if (parts.length > 1) {
        result = parseInt(parts[1]) || UNEXPECTED_ERROR;
      }
      Services.telemetry.getHistogramById("UPDATER_STATUS_CODES").add(result);
    } catch(e) {
      
      Components.utils.reportError(e);
    }
  },

  




  notify: function AUS_notify(timer) {
    
    if (this.isDownloading || this._downloader && this._downloader.patchIsStaged)
      return;

    var self = this;
    var listener = {
      


      onProgress: function AUS_notify_onProgress(request, position, totalSize) {
      },

      


      onCheckComplete: function AUS_notify_onCheckComplete(request, updates,
                                                           updateCount) {
        self._selectAndInstallUpdate(updates);
      },

      


      onError: function AUS_notify_onError(request, update) {
        LOG("UpdateService:notify:listener - error during background update: " +
            update.statusText);

        var maxErrors;
        var errCount;
        if (update.errorCode == CERT_ATTR_CHECK_FAILED_NO_UPDATE ||
            update.errorCode == CERT_ATTR_CHECK_FAILED_HAS_UPDATE) {
          errCount = getPref("getIntPref", PREF_APP_UPDATE_CERT_ERRORS, 0);
          errCount++;
          Services.prefs.setIntPref(PREF_APP_UPDATE_CERT_ERRORS, errCount);
          maxErrors = getPref("getIntPref", PREF_APP_UPDATE_CERT_MAXERRORS, 5);
        }
        else {
          update.errorCode = BACKGROUNDCHECK_MULTIPLE_FAILURES;
          errCount = getPref("getIntPref", PREF_APP_UPDATE_BACKGROUNDERRORS, 0);
          errCount++;
          Services.prefs.setIntPref(PREF_APP_UPDATE_BACKGROUNDERRORS, errCount);
          maxErrors = getPref("getIntPref", PREF_APP_UPDATE_BACKGROUNDMAXERRORS,
                              10);
        }

        if (errCount >= maxErrors) {
          var prompter = Cc["@mozilla.org/updates/update-prompt;1"].
                         createInstance(Ci.nsIUpdatePrompt);
          prompter.showUpdateError(update);
        }
      }
    };
    this.backgroundChecker.checkForUpdates(listener, false);
  },

  







  selectUpdate: function AUS_selectUpdate(updates) {
    if (updates.length == 0)
      return null;

    
    var majorUpdate = null;
    var minorUpdate = null;
    var vc = Services.vc;

    updates.forEach(function(aUpdate) {
      
      
      if (vc.compare(aUpdate.appVersion, Services.appinfo.version) < 0 ||
          vc.compare(aUpdate.appVersion, Services.appinfo.version) == 0 &&
          aUpdate.buildID == Services.appinfo.appBuildID) {
        LOG("UpdateService:selectUpdate - skipping update because the " +
            "update's application version is less than the current " +
            "application version");
        return;
      }

      
      
      
      let neverPrefName = PREF_APP_UPDATE_NEVER_BRANCH + aUpdate.appVersion;
      if (aUpdate.showNeverForVersion &&
          getPref("getBoolPref", neverPrefName, false)) {
        LOG("UpdateService:selectUpdate - skipping update because the " +
            "preference " + neverPrefName + " is true");
        return;
      }

      switch (aUpdate.type) {
        case "major":
          if (!majorUpdate)
            majorUpdate = aUpdate;
          else if (vc.compare(majorUpdate.appVersion, aUpdate.appVersion) <= 0)
            majorUpdate = aUpdate;
          break;
        case "minor":
          if (!minorUpdate)
            minorUpdate = aUpdate;
          else if (vc.compare(minorUpdate.appVersion, aUpdate.appVersion) <= 0)
            minorUpdate = aUpdate;
          break;
        default:
          LOG("UpdateService:selectUpdate - skipping unknown update type: " +
              aUpdate.type);
          break;
      }
    });

    return minorUpdate || majorUpdate;
  },

  



  _update: null,

  





  _selectAndInstallUpdate: function AUS__selectAndInstallUpdate(updates) {
    
    
    var um = Cc["@mozilla.org/updates/update-manager;1"].
             getService(Ci.nsIUpdateManager);
    if (um.activeUpdate)
      return;

    var update = this.selectUpdate(updates, updates.length);
    if (!update)
      return;

    var updateEnabled = getPref("getBoolPref", PREF_APP_UPDATE_ENABLED, true);
    if (!updateEnabled) {
      LOG("UpdateService:_selectAndInstallUpdate - not prompting because " +
          "update is disabled");
      return;
    }

    if (!gCanApplyUpdates) {
      LOG("UpdateService:_selectAndInstallUpdate - the user is unable to " +
          "apply updates... prompting");
      this._showPrompt(update);
      return;
    }

    

























    if (update.showPrompt) {
      LOG("UpdateService:_selectAndInstallUpdate - prompting because the " +
          "update snippet specified showPrompt");
      this._showPrompt(update);
      return;
    }

    if (!getPref("getBoolPref", PREF_APP_UPDATE_AUTO, true)) {
      LOG("UpdateService:_selectAndInstallUpdate - prompting because silent " +
          "install is disabled");
      this._showPrompt(update);
      return;
    }

    if (getPref("getIntPref", PREF_APP_UPDATE_MODE, 1) == 0) {
      
      LOG("UpdateService:_selectAndInstallUpdate - no need to show prompt, " +
          "just download the update");
      var status = this.downloadUpdate(update, true);
      if (status == STATE_NONE)
        cleanupActiveUpdate();
      return;
    }

    
    if (update.appVersion &&
        Services.vc.compare(update.appVersion, Services.appinfo.version) != 0) {
      this._update = update;
      this._checkAddonCompatibility();
    }
    else {
      LOG("UpdateService:_selectAndInstallUpdate - no need to show prompt, " +
          "just download the update");
      var status = this.downloadUpdate(update, true);
      if (status == STATE_NONE)
        cleanupActiveUpdate();
    }
  },

  _showPrompt: function AUS__showPrompt(update) {
    var prompter = Cc["@mozilla.org/updates/update-prompt;1"].
                   createInstance(Ci.nsIUpdatePrompt);
    prompter.showUpdateAvailable(update);
  },

  _checkAddonCompatibility: function AUS__checkAddonCompatibility() {
    try {
      var hotfixID = Services.prefs.getCharPref(PREF_EM_HOTFIX_ID);
    }
    catch (e) { }

    
    var self = this;
    AddonManager.getAllAddons(function(addons) {
      self._incompatibleAddons = [];
      addons.forEach(function(addon) {
        
        
        if (!("isCompatibleWith" in addon) || !("findUpdates" in addon)) {
          let errMsg = "Add-on doesn't implement either the isCompatibleWith " +
                       "or the findUpdates method!";
          if (addon.id)
            errMsg += " Add-on ID: " + addon.id;
          Components.utils.reportError(errMsg);
          return;
        }

        
        
        
        
        
        
        
        
        
        
        try {
          if (addon.type != "plugin" && addon.id != hotfixID &&
              !addon.appDisabled && !addon.userDisabled &&
              addon.scope != AddonManager.SCOPE_APPLICATION &&
              addon.isCompatible &&
              !addon.isCompatibleWith(self._update.appVersion,
                                      self._update.platformVersion))
            self._incompatibleAddons.push(addon);
        }
        catch (e) {
          Components.utils.reportError(e);
        }
      });

      if (self._incompatibleAddons.length > 0) {
      



















        self._updateCheckCount = self._incompatibleAddons.length;
        LOG("UpdateService:_checkAddonCompatibility - checking for " +
            "incompatible add-ons");

        self._incompatibleAddons.forEach(function(addon) {
          addon.findUpdates(this, AddonManager.UPDATE_WHEN_NEW_APP_DETECTED,
                            this._update.appVersion, this._update.platformVersion);
        }, self);
      }
      else {
        LOG("UpdateService:_checkAddonCompatibility - no need to show prompt, " +
            "just download the update");
        var status = self.downloadUpdate(self._update, true);
        if (status == STATE_NONE)
          cleanupActiveUpdate();
        self._update = null;
      }
    });
  },

  
  onCompatibilityUpdateAvailable: function(addon) {
    
    
    for (var i = 0; i < this._incompatibleAddons.length; ++i) {
      if (this._incompatibleAddons[i].id == addon.id) {
        LOG("UpdateService:onAddonUpdateEnded - found update for add-on ID: " +
            addon.id);
        this._incompatibleAddons.splice(i, 1);
      }
    }
  },

  onUpdateAvailable: function(addon, install) {
    if (getPref("getIntPref", PREF_APP_UPDATE_INCOMPATIBLE_MODE, 0) == 1)
      return;

    
    
    
    let bs = Cc["@mozilla.org/extensions/blocklist;1"].
             getService(Ci.nsIBlocklistService);
    if (bs.isAddonBlocklisted(addon.id, install.version,
                              gUpdates.update.appVersion,
                              gUpdates.update.platformVersion))
      return;

    
    this.onCompatibilityUpdateAvailable(addon);
  },

  onUpdateFinished: function(addon) {
    if (--this._updateCheckCount > 0)
      return;

    if (this._incompatibleAddons.length > 0 || !gCanApplyUpdates) {
      LOG("UpdateService:onUpdateEnded - prompting because there are " +
          "incompatible add-ons");
      this._showPrompt(this._update);
    }
    else {
      LOG("UpdateService:onUpdateEnded - no need to show prompt, just " +
          "download the update");
      var status = this.downloadUpdate(this._update, true);
      if (status == STATE_NONE)
        cleanupActiveUpdate();
    }
    this._update = null;
  },

  


  _backgroundChecker: null,

  


  get backgroundChecker() {
    if (!this._backgroundChecker)
      this._backgroundChecker = new Checker();
    return this._backgroundChecker;
  },

  


  get canCheckForUpdates() {
    return gCanCheckForUpdates;
  },

  


  get canApplyUpdates() {
    return gCanApplyUpdates;
  },

  


  get canStageUpdates() {
    return gCanStageUpdates;
  },

  


  addDownloadListener: function AUS_addDownloadListener(listener) {
    if (!this._downloader) {
      LOG("UpdateService:addDownloadListener - no downloader!");
      return;
    }
    this._downloader.addDownloadListener(listener);
  },

  


  removeDownloadListener: function AUS_removeDownloadListener(listener) {
    if (!this._downloader) {
      LOG("UpdateService:removeDownloadListener - no downloader!");
      return;
    }
    this._downloader.removeDownloadListener(listener);
  },

  


  downloadUpdate: function AUS_downloadUpdate(update, background) {
    if (!update)
      throw Cr.NS_ERROR_NULL_POINTER;

    
    
    
    
    if (update.appVersion &&
        (Services.vc.compare(update.appVersion, Services.appinfo.version) < 0 ||
         update.buildID && update.buildID == Services.appinfo.appBuildID &&
         update.appVersion == Services.appinfo.version)) {
      LOG("UpdateService:downloadUpdate - canceling download of update since " +
          "it is for an earlier or same application version and build ID.\n" +
          "current application version: " + Services.appinfo.version + "\n" +
          "update application version : " + update.appVersion + "\n" +
          "current build ID: " + Services.appinfo.appBuildID + "\n" +
          "update build ID : " + update.buildID);
      cleanupActiveUpdate();
      return STATE_NONE;
    }

    
    if (this.isDownloading) {
      if (update.isCompleteUpdate == this._downloader.isCompleteUpdate &&
          background == this._downloader.background) {
        LOG("UpdateService:downloadUpdate - no support for downloading more " +
            "than one update at a time");
        return readStatusFile(getUpdatesDir());
      }
      this._downloader.cancel();
    }
    
    update.previousAppVersion = Services.appinfo.version;
    this._downloader = new Downloader(background);
    return this._downloader.downloadUpdate(update);
  },

  applyUpdateInBackground: function AUS_applyUpdateInBackground(update) {
    
    if (!gCanStageUpdates) {
      return;
    }

    LOG("UpdateService:applyUpdateInBackground called with the following update: " +
        update.name);

    
    try {
      Cc["@mozilla.org/updates/update-processor;1"].
        createInstance(Ci.nsIUpdateProcessor).
        processUpdate(update);
    } catch (e) {
      
      
    }
  },

  


  pauseDownload: function AUS_pauseDownload() {
    if (this.isDownloading)
      this._downloader.cancel();
  },

  


  getUpdatesDirectory: getUpdatesDir,

  


  get isDownloading() {
    return this._downloader && this._downloader.isBusy;
  },

  classID: UPDATESERVICE_CID,
  classInfo: XPCOMUtils.generateCI({classID: UPDATESERVICE_CID,
                                    contractID: UPDATESERVICE_CONTRACTID,
                                    interfaces: [Ci.nsIApplicationUpdateService,
                                                 Ci.nsITimerCallback,
                                                 Ci.nsIObserver],
                                    flags: Ci.nsIClassInfo.SINGLETON}),

  _xpcom_factory: UpdateServiceFactory,
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIApplicationUpdateService,
                                         Ci.nsIAddonUpdateCheckListener,
                                         Ci.nsITimerCallback,
                                         Ci.nsIObserver])
};





function UpdateManager() {
  
  var updates = this._loadXMLFileIntoArray(getUpdateFile(
                  [FILE_UPDATE_ACTIVE]));
  if (updates.length > 0) {
    
    
    
    
    if (readStatusFile(getUpdatesDir()) == STATE_NONE) {
      cleanUpUpdatesDir();
      this._writeUpdatesToXMLFile([], getUpdateFile([FILE_UPDATE_ACTIVE]));
    }
    else
      this._activeUpdate = updates[0];
  }
}
UpdateManager.prototype = {
  



  _updates: null,

  


  _activeUpdate: null,

  








  observe: function UM_observe(subject, topic, data) {
    
    if (topic == "um-reload-update-data") {
      this._updates = this._loadXMLFileIntoArray(getUpdateFile(
                        [FILE_UPDATES_DB]));
      this._activeUpdate = null;
      var updates = this._loadXMLFileIntoArray(getUpdateFile(
                      [FILE_UPDATE_ACTIVE]));
      if (updates.length > 0)
        this._activeUpdate = updates[0];
    }
  },

  





  _loadXMLFileIntoArray: function UM__loadXMLFileIntoArray(file) {
    if (!file.exists()) {
      LOG("UpdateManager:_loadXMLFileIntoArray: XML file does not exist");
      return [];
    }

    var result = [];
    var fileStream = Cc["@mozilla.org/network/file-input-stream;1"].
                     createInstance(Ci.nsIFileInputStream);
    fileStream.init(file, FileUtils.MODE_RDONLY, FileUtils.PERMS_FILE, 0);
    try {
      var parser = Cc["@mozilla.org/xmlextras/domparser;1"].
                   createInstance(Ci.nsIDOMParser);
      var doc = parser.parseFromStream(fileStream, "UTF-8",
                                       fileStream.available(), "text/xml");

      const ELEMENT_NODE = Ci.nsIDOMNode.ELEMENT_NODE;
      var updateCount = doc.documentElement.childNodes.length;
      for (var i = 0; i < updateCount; ++i) {
        var updateElement = doc.documentElement.childNodes.item(i);
        if (updateElement.nodeType != ELEMENT_NODE ||
            updateElement.localName != "update")
          continue;

        updateElement.QueryInterface(Ci.nsIDOMElement);
        try {
          var update = new Update(updateElement);
        } catch (e) {
          LOG("UpdateManager:_loadXMLFileIntoArray - invalid update");
          continue;
        }
        result.push(update);
      }
    }
    catch (e) {
      LOG("UpdateManager:_loadXMLFileIntoArray - error constructing update " +
          "list. Exception: " + e);
    }
    fileStream.close();
    return result;
  },

  


  _ensureUpdates: function UM__ensureUpdates() {
    if (!this._updates) {
      this._updates = this._loadXMLFileIntoArray(getUpdateFile(
                        [FILE_UPDATES_DB]));
      var activeUpdates = this._loadXMLFileIntoArray(getUpdateFile(
                            [FILE_UPDATE_ACTIVE]));
      if (activeUpdates.length > 0)
        this._activeUpdate = activeUpdates[0];
    }
  },

  


  getUpdateAt: function UM_getUpdateAt(index) {
    this._ensureUpdates();
    return this._updates[index];
  },

  


  get updateCount() {
    this._ensureUpdates();
    return this._updates.length;
  },

  


  get activeUpdate() {
    if (this._activeUpdate &&
        this._activeUpdate.channel != getUpdateChannel()) {
      
      
      this._activeUpdate = null;
      this.saveUpdates();

      
      cleanUpUpdatesDir();
    }
    return this._activeUpdate;
  },
  set activeUpdate(activeUpdate) {
    this._addUpdate(activeUpdate);
    this._activeUpdate = activeUpdate;
    if (!activeUpdate) {
      
      
      this.saveUpdates();
    }
    else
      this._writeUpdatesToXMLFile([this._activeUpdate],
                                  getUpdateFile([FILE_UPDATE_ACTIVE]));
    return activeUpdate;
  },

  





  _addUpdate: function UM__addUpdate(update) {
    if (!update)
      return;
    this._ensureUpdates();
    if (this._updates) {
      for (var i = 0; i < this._updates.length; ++i) {
        if (this._updates[i] &&
            this._updates[i].appVersion == update.appVersion &&
            this._updates[i].buildID == update.buildID) {
          
          
          this._updates[i] = update;
          return;
        }
      }
    }
    
    this._updates.unshift(update);
  },

  






  _writeUpdatesToXMLFile: function UM__writeUpdatesToXMLFile(updates, file) {
    var fos = Cc["@mozilla.org/network/safe-file-output-stream;1"].
              createInstance(Ci.nsIFileOutputStream);
    var modeFlags = FileUtils.MODE_WRONLY | FileUtils.MODE_CREATE |
                    FileUtils.MODE_TRUNCATE;
    if (!file.exists())
      file.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
    fos.init(file, modeFlags, FileUtils.PERMS_FILE, 0);

    try {
      var parser = Cc["@mozilla.org/xmlextras/domparser;1"].
                   createInstance(Ci.nsIDOMParser);
      const EMPTY_UPDATES_DOCUMENT = "<?xml version=\"1.0\"?><updates xmlns=\"http://www.mozilla.org/2005/app-update\"></updates>";
      var doc = parser.parseFromString(EMPTY_UPDATES_DOCUMENT, "text/xml");

      for (var i = 0; i < updates.length; ++i) {
        if (updates[i])
          doc.documentElement.appendChild(updates[i].serialize(doc));
      }

      var serializer = Cc["@mozilla.org/xmlextras/xmlserializer;1"].
                       createInstance(Ci.nsIDOMSerializer);
      serializer.serializeToStream(doc.documentElement, fos, null);
    }
    catch (e) {
    }

    FileUtils.closeSafeFileOutputStream(fos);
  },

  


  saveUpdates: function UM_saveUpdates() {
    this._writeUpdatesToXMLFile([this._activeUpdate],
                                getUpdateFile([FILE_UPDATE_ACTIVE]));
    if (this._activeUpdate)
      this._addUpdate(this._activeUpdate);

    this._ensureUpdates();
    
    if (this._updates) {
      let updates = this._updates.slice();
      for (let i = updates.length - 1; i >= 0; --i) {
        let state = updates[i].state;
        if (state == STATE_NONE || state == STATE_DOWNLOADING ||
            state == STATE_APPLIED || state == STATE_APPLIED_SVC ||
            state == STATE_PENDING || state == STATE_PENDING_SVC) {
          updates.splice(i, 1);
        }
      }

      this._writeUpdatesToXMLFile(updates.slice(0, 10),
                                  getUpdateFile([FILE_UPDATES_DB]));
    }
  },

  refreshUpdateStatus: function UM_refreshUpdateStatus(update) {
    var updateSucceeded = true;
    var status = readStatusFile(getUpdatesDir());
    var ary = status.split(":");
    update.state = ary[0];
    if (update.state == STATE_FAILED && ary[1]) {
      updateSucceeded = false;
      if (!handleUpdateFailure(update, ary[1])) {
        handleFallbackToCompleteUpdate(update, true);
      }
    }
    if (update.state == STATE_APPLIED && shouldUseService()) {
      writeStatusFile(getUpdatesDir(), update.state = STATE_APPLIED_SVC);
    }
    var um = Cc["@mozilla.org/updates/update-manager;1"].
             getService(Ci.nsIUpdateManager);
    um.saveUpdates();

    if (update.state != STATE_PENDING && update.state != STATE_PENDING_SVC) {
      
      
      
      cleanUpUpdatesDir(updateSucceeded);
    }

    
    
    LOG("UpdateManager:refreshUpdateStatus - Notifying observers that " +
        "the update was staged. state: " + update.state + ", status: " + status);
    Services.obs.notifyObservers(null, "update-staged", update.state);

    
    
    if (update.state == STATE_APPLIED) {
      
      
      
      var prompter = Cc["@mozilla.org/updates/update-prompt;1"].
                     createInstance(Ci.nsIUpdatePrompt);
      prompter.showUpdateDownloaded(update, true);
    }
  },

  classID: Components.ID("{093C2356-4843-4C65-8709-D7DBCBBE7DFB}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdateManager, Ci.nsIObserver])
};






function Checker() {
}
Checker.prototype = {
  


  _request  : null,

  


  _callback : null,

  



  getUpdateURL: function UC_getUpdateURL(force) {
    this._forced = force;

    
    var url = getPref("getCharPref", PREF_APP_UPDATE_URL_OVERRIDE, null);

    
    if (!url) {
      try {
        url = Services.prefs.getDefaultBranch(null).
              getCharPref(PREF_APP_UPDATE_URL);
      } catch (e) {
      }
    }

    if (!url || url == "") {
      LOG("Checker:getUpdateURL - update URL not defined");
      return null;
    }

    url = url.replace(/%PRODUCT%/g, Services.appinfo.name);
    url = url.replace(/%VERSION%/g, Services.appinfo.version);
    url = url.replace(/%BUILD_ID%/g, Services.appinfo.appBuildID);
    url = url.replace(/%BUILD_TARGET%/g, Services.appinfo.OS + "_" + gABI);
    url = url.replace(/%OS_VERSION%/g, gOSVersion);
    if (/%LOCALE%/.test(url))
      url = url.replace(/%LOCALE%/g, getLocale());
    url = url.replace(/%CHANNEL%/g, getUpdateChannel());
    url = url.replace(/%PLATFORM_VERSION%/g, Services.appinfo.platformVersion);
    url = url.replace(/%DISTRIBUTION%/g,
                      getDistributionPrefValue(PREF_APP_DISTRIBUTION));
    url = url.replace(/%DISTRIBUTION_VERSION%/g,
                      getDistributionPrefValue(PREF_APP_DISTRIBUTION_VERSION));
    url = url.replace(/\+/g, "%2B");

    if (force)
      url += (url.indexOf("?") != -1 ? "&" : "?") + "force=1";

    LOG("Checker:getUpdateURL - update URL: " + url);
    return url;
  },

  


  checkForUpdates: function UC_checkForUpdates(listener, force) {
    if (!listener)
      throw Cr.NS_ERROR_NULL_POINTER;

    var url = this.getUpdateURL(force);
    if (!url || (!this.enabled && !force))
      return;

    this._request = Cc["@mozilla.org/xmlextras/xmlhttprequest;1"].
                    createInstance(Ci.nsISupports);
    
    if (this._request.wrappedJSObject) {
      this._request = this._request.wrappedJSObject;
    }
    this._request.open("GET", url, true);
    var allowNonBuiltIn = !getPref("getBoolPref",
                                   PREF_APP_UPDATE_CERT_REQUIREBUILTIN, true);
    this._request.channel.notificationCallbacks = new gCertUtils.BadCertHandler(allowNonBuiltIn);
    this._request.overrideMimeType("text/xml");
    this._request.setRequestHeader("Cache-Control", "no-cache");

    var self = this;
    this._request.addEventListener("error", function(event) { self.onError(event); } ,false);
    this._request.addEventListener("load", function(event) { self.onLoad(event); }, false);
    this._request.addEventListener("progress", function(event) { self.onProgress(event); }, false);

    LOG("Checker:checkForUpdates - sending request to: " + url);
    this._request.send(null);

    this._callback = listener;
  },

  




  onProgress: function UC_onProgress(event) {
    LOG("Checker:onProgress - " + event.position + "/" + event.totalSize);
    this._callback.onProgress(event.target, event.position, event.totalSize);
  },

  



  get _updates() {
    var updatesElement = this._request.responseXML.documentElement;
    if (!updatesElement) {
      LOG("Checker:_updates get - empty updates document?!");
      return [];
    }

    if (updatesElement.nodeName != "updates") {
      LOG("Checker:updates get - unexpected node name!");
      throw new Error("Unexpected node name, expected: updates, got: " +
                      updatesElement.nodeName);
    }

    const ELEMENT_NODE = Ci.nsIDOMNode.ELEMENT_NODE;
    var updates = [];
    for (var i = 0; i < updatesElement.childNodes.length; ++i) {
      var updateElement = updatesElement.childNodes.item(i);
      if (updateElement.nodeType != ELEMENT_NODE ||
          updateElement.localName != "update")
        continue;

      updateElement.QueryInterface(Ci.nsIDOMElement);
      try {
        var update = new Update(updateElement);
      } catch (e) {
        LOG("Checker:updates get - invalid <update/>, ignoring...");
        continue;
      }
      update.serviceURL = this.getUpdateURL(this._forced);
      update.channel = getUpdateChannel();
      updates.push(update);
    }

    return updates;
  },

  


  _getChannelStatus: function UC__getChannelStatus(request) {
    var status = 0;
    try {
      status = request.status;
    }
    catch (e) {
    }

    if (status == 0)
      status = request.channel.QueryInterface(Ci.nsIRequest).status;
    return status;
  },

  




  onLoad: function UC_onLoad(event) {
    LOG("Checker:onLoad - request completed downloading document");

    var prefs = Services.prefs;
    var certs = null;
    if (!prefs.prefHasUserValue(PREF_APP_UPDATE_URL_OVERRIDE) &&
        getPref("getBoolPref", PREF_APP_UPDATE_CERT_CHECKATTRS, true)) {
      certs = gCertUtils.readCertPrefs(PREF_APP_UPDATE_CERTS_BRANCH);
    }

    try {
      
      var updates = this._updates;
      LOG("Checker:onLoad - number of updates available: " + updates.length);
      var allowNonBuiltIn = !getPref("getBoolPref",
                                     PREF_APP_UPDATE_CERT_REQUIREBUILTIN, true);
      gCertUtils.checkCert(this._request.channel, allowNonBuiltIn, certs);

      if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_CERT_ERRORS))
        Services.prefs.clearUserPref(PREF_APP_UPDATE_CERT_ERRORS);

      if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_BACKGROUNDERRORS))
        Services.prefs.clearUserPref(PREF_APP_UPDATE_BACKGROUNDERRORS);

      
      this._callback.onCheckComplete(event.target, updates, updates.length);
    }
    catch (e) {
      LOG("Checker:onLoad - there was a problem checking for updates. " +
          "Exception: " + e);
      var request = event.target;
      var status = this._getChannelStatus(request);
      LOG("Checker:onLoad - request.status: " + status);
      var update = new Update(null);
      update.statusText = getStatusTextFromCode(status, 404);
      if (e.result == Cr.NS_ERROR_ILLEGAL_VALUE) {
        update.errorCode = updates[0] ? CERT_ATTR_CHECK_FAILED_HAS_UPDATE
                                      : CERT_ATTR_CHECK_FAILED_NO_UPDATE;
      }
      this._callback.onError(request, update);
    }

    this._callback = null;
    this._request = null;
  },

  




  onError: function UC_onError(event) {
    var request = event.target;
    var status = this._getChannelStatus(request);
    LOG("Checker:onError - request.status: " + status);

    
    
    
    var update = new Update(null);
    update.statusText = getStatusTextFromCode(status, 200);
    this._callback.onError(request, update);

    this._request = null;
  },

  


  _enabled: true,
  get enabled() {
    return getPref("getBoolPref", PREF_APP_UPDATE_ENABLED, true) &&
           gCanCheckForUpdates && this._enabled;
  },

  


  stopChecking: function UC_stopChecking(duration) {
    
    if (this._request)
      this._request.abort();

    switch (duration) {
    case Ci.nsIUpdateChecker.CURRENT_SESSION:
      this._enabled = false;
      break;
    case Ci.nsIUpdateChecker.ANY_CHECKS:
      this._enabled = false;
      Services.prefs.setBoolPref(PREF_APP_UPDATE_ENABLED, this._enabled);
      break;
    }

    this._callback = null;
  },

  classID: Components.ID("{898CDC9B-E43F-422F-9CC4-2F6291B415A3}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdateChecker])
};








function Downloader(background) {
  this.background = background;
}
Downloader.prototype = {
  


  _patch: null,

  


  _update: null,

  


  _request: null,

  




  isCompleteUpdate: null,

  


  cancel: function Downloader_cancel() {
    if (this._request && this._request instanceof Ci.nsIRequest)
      this._request.cancel(Cr.NS_BINDING_ABORTED);
  },

  


  get patchIsStaged() {
    var readState = readStatusFile(getUpdatesDir());
    
    
    
    return readState == STATE_PENDING || readState == STATE_PENDING_SVC ||
           readState == STATE_APPLIED || readState == STATE_APPLIED_SVC;
  },

  



  _verifyDownload: function Downloader__verifyDownload() {
    if (!this._request)
      return false;

    var destination = this._request.destination;

    
    if (destination.fileSize != this._patch.size)
      return false;

    var fileStream = Cc["@mozilla.org/network/file-input-stream;1"].
                     createInstance(Ci.nsIFileInputStream);
    fileStream.init(destination, FileUtils.MODE_RDONLY, FileUtils.PERMS_FILE, 0);

    try {
      var hash = Cc["@mozilla.org/security/hash;1"].
                 createInstance(Ci.nsICryptoHash);
      var hashFunction = Ci.nsICryptoHash[this._patch.hashFunction.toUpperCase()];
      if (hashFunction == undefined)
        throw Cr.NS_ERROR_UNEXPECTED;
      hash.init(hashFunction);
      hash.updateFromStream(fileStream, -1);
      
      
      
      
      digest = binaryToHex(hash.finish(false));
    } catch (e) {
      LOG("Downloader:_verifyDownload - failed to compute hash of the " +
          "downloaded update archive");
      digest = "";
    }

    fileStream.close();

    return digest == this._patch.hashValue.toLowerCase();
  },

  








  _selectPatch: function Downloader__selectPatch(update, updateDir) {
    
    

    





    function getPatchOfType(type) {
      for (var i = 0; i < update.patchCount; ++i) {
        var patch = update.getPatchAt(i);
        if (patch && patch.type == type)
          return patch;
      }
      return null;
    }

    
    
    
    var selectedPatch = update.selectedPatch;

    var state = readStatusFile(updateDir);

    
    
    var useComplete = false;
    if (selectedPatch) {
      LOG("Downloader:_selectPatch - found existing patch with state: " +
          state);
      switch (state) {
      case STATE_DOWNLOADING:
        LOG("Downloader:_selectPatch - resuming download");
        return selectedPatch;
      case STATE_PENDING_SVC:
      case STATE_PENDING:
        LOG("Downloader:_selectPatch - already downloaded and staged");
        return null;
      default:
        
        
        if (update && selectedPatch.type == "partial") {
          useComplete = true;
        } else {
          
          LOG("Downloader:_selectPatch - failed to apply complete patch!");
          writeStatusFile(updateDir, STATE_NONE);
          writeVersionFile(getUpdatesDir(), null);
          return null;
        }
      }

      selectedPatch = null;
    }

    
    
    var partialPatch = getPatchOfType("partial");
    if (!useComplete)
      selectedPatch = partialPatch;
    if (!selectedPatch) {
      if (partialPatch)
        partialPatch.selected = false;
      selectedPatch = getPatchOfType("complete");
    }

    
    updateDir = getUpdatesDir();

    
    
    
    if (selectedPatch)
      selectedPatch.selected = true;

    update.isCompleteUpdate = useComplete;

    
    
    var um = Cc["@mozilla.org/updates/update-manager;1"].
             getService(Ci.nsIUpdateManager);
    um.activeUpdate = update;

    return selectedPatch;
  },

  


  get isBusy() {
    return this._request != null;
  },

  




  downloadUpdate: function Downloader_downloadUpdate(update) {
    if (!update)
      throw Cr.NS_ERROR_NULL_POINTER;

    var updateDir = getUpdatesDir();

    this._update = update;

    
    
    this._patch = this._selectPatch(update, updateDir);
    if (!this._patch) {
      LOG("Downloader:downloadUpdate - no patch to download");
      return readStatusFile(updateDir);
    }
    this.isCompleteUpdate = this._patch.type == "complete";

    var patchFile = updateDir.clone();
    patchFile.append(FILE_UPDATE_ARCHIVE);

    var uri = Services.io.newURI(this._patch.URL, null, null);

    this._request = Cc["@mozilla.org/network/incremental-download;1"].
                    createInstance(Ci.nsIIncrementalDownload);

    LOG("Downloader:downloadUpdate - downloading from " + uri.spec + " to " +
        patchFile.path);
    var interval = this.background ? getPref("getIntPref",
                                             PREF_APP_UPDATE_BACKGROUND_INTERVAL,
                                             DOWNLOAD_BACKGROUND_INTERVAL)
                                   : DOWNLOAD_FOREGROUND_INTERVAL;
    this._request.init(uri, patchFile, DOWNLOAD_CHUNK_SIZE, interval);
    this._request.start(this, null);

    writeStatusFile(updateDir, STATE_DOWNLOADING);
    this._patch.QueryInterface(Ci.nsIWritablePropertyBag);
    this._patch.state = STATE_DOWNLOADING;
    var um = Cc["@mozilla.org/updates/update-manager;1"].
             getService(Ci.nsIUpdateManager);
    um.saveUpdates();
    return STATE_DOWNLOADING;
  },

  



  _listeners: [],

  





  addDownloadListener: function Downloader_addDownloadListener(listener) {
    for (var i = 0; i < this._listeners.length; ++i) {
      if (this._listeners[i] == listener)
        return;
    }
    this._listeners.push(listener);
  },

  




  removeDownloadListener: function Downloader_removeDownloadListener(listener) {
    for (var i = 0; i < this._listeners.length; ++i) {
      if (this._listeners[i] == listener) {
        this._listeners.splice(i, 1);
        return;
      }
    }
  },

  






  onStartRequest: function Downloader_onStartRequest(request, context) {
    if (request instanceof Ci.nsIIncrementalDownload)
      LOG("Downloader:onStartRequest - original URI spec: " + request.URI.spec +
          ", final URI spec: " + request.finalURI.spec);
    
    this._patch.finalURL = request.finalURI.spec;
    var um = Cc["@mozilla.org/updates/update-manager;1"].
             getService(Ci.nsIUpdateManager);
    um.saveUpdates();

    var listeners = this._listeners.concat();
    var listenerCount = listeners.length;
    for (var i = 0; i < listenerCount; ++i)
      listeners[i].onStartRequest(request, context);
  },

  










  onProgress: function Downloader_onProgress(request, context, progress,
                                             maxProgress) {
    LOG("Downloader:onProgress - progress: " + progress + "/" + maxProgress);

    var listeners = this._listeners.concat();
    var listenerCount = listeners.length;
    for (var i = 0; i < listenerCount; ++i) {
      var listener = listeners[i];
      if (listener instanceof Ci.nsIProgressEventSink)
        listener.onProgress(request, context, progress, maxProgress);
    }
  },

  










  onStatus: function Downloader_onStatus(request, context, status, statusText) {
    LOG("Downloader:onStatus - status: " + status + ", statusText: " +
        statusText);

    var listeners = this._listeners.concat();
    var listenerCount = listeners.length;
    for (var i = 0; i < listenerCount; ++i) {
      var listener = listeners[i];
      if (listener instanceof Ci.nsIProgressEventSink)
        listener.onStatus(request, context, status, statusText);
    }
  },

  








  onStopRequest: function  Downloader_onStopRequest(request, context, status) {
    if (request instanceof Ci.nsIIncrementalDownload)
      LOG("Downloader:onStopRequest - original URI spec: " + request.URI.spec +
          ", final URI spec: " + request.finalURI.spec + ", status: " + status);

    
    
    var state = this._patch.state;
    var shouldShowPrompt = false;
    var deleteActiveUpdate = false;
    if (Components.isSuccessCode(status)) {
      if (this._verifyDownload()) {
        state = shouldUseService() ? STATE_PENDING_SVC : STATE_PENDING

        
        
        
        if (this.background)
          shouldShowPrompt = !getPref("getBoolPref", PREF_APP_UPDATE_STAGE_ENABLED, false);

        
        writeStatusFile(getUpdatesDir(), state);
        writeVersionFile(getUpdatesDir(), this._update.appVersion);
        this._update.installDate = (new Date()).getTime();
        this._update.statusText = gUpdateBundle.GetStringFromName("installPending");
      }
      else {
        LOG("Downloader:onStopRequest - download verification failed");
        state = STATE_DOWNLOAD_FAILED;

        
        status = Cr.NS_ERROR_UNEXPECTED;

        
        const vfCode = "verification_failed";
        var message = getStatusTextFromCode(vfCode, vfCode);
        this._update.statusText = message;

        if (this._update.isCompleteUpdate || this._update.patchCount != 2)
          deleteActiveUpdate = true;

        
        cleanUpUpdatesDir();
      }
    }
    else if (status != Cr.NS_BINDING_ABORTED &&
             status != Cr.NS_ERROR_ABORT &&
             status != Cr.NS_ERROR_DOCUMENT_NOT_CACHED) {
      LOG("Downloader:onStopRequest - non-verification failure");
      
      state = STATE_DOWNLOAD_FAILED;

      
      

      this._update.statusText = getStatusTextFromCode(status,
                                                      Cr.NS_BINDING_FAILED);

      
      cleanUpUpdatesDir();

      deleteActiveUpdate = true;
    }
    LOG("Downloader:onStopRequest - setting state to: " + state);
    this._patch.state = state;
    var um = Cc["@mozilla.org/updates/update-manager;1"].
             getService(Ci.nsIUpdateManager);
    if (deleteActiveUpdate) {
      this._update.installDate = (new Date()).getTime();
      um.activeUpdate = null;
    }
    else {
      if (um.activeUpdate)
        um.activeUpdate.state = state;
    }
    um.saveUpdates();

    var listeners = this._listeners.concat();
    var listenerCount = listeners.length;
    for (var i = 0; i < listenerCount; ++i)
      listeners[i].onStopRequest(request, context, status);

    this._request = null;

    if (state == STATE_DOWNLOAD_FAILED) {
      var allFailed = true;
      
      if (!this._update.isCompleteUpdate && this._update.patchCount == 2) {
        LOG("Downloader:onStopRequest - verification of patch failed, " +
            "downloading complete update patch");
        this._update.isCompleteUpdate = true;
        var status = this.downloadUpdate(this._update);

        if (status == STATE_NONE) {
          cleanupActiveUpdate();
        } else {
          allFailed = false;
        }
      }

      if (allFailed) {
        LOG("Downloader:onStopRequest - all update patch downloads failed");
        
        
        
        
        if (!Services.wm.getMostRecentWindow(UPDATE_WINDOW_NAME)) {
          try {
            this._update.QueryInterface(Ci.nsIWritablePropertyBag);
            var fgdl = this._update.getProperty("foregroundDownload");
          }
          catch (e) {
          }

          if (fgdl == "true") {
            var prompter = Cc["@mozilla.org/updates/update-prompt;1"].
                           createInstance(Ci.nsIUpdatePrompt);
            prompter.showUpdateError(this._update);
          }
        }
        
        this._update = null;
      }
      
      return;
    }

    
    
    if (shouldShowPrompt) {
      
      
      
      var prompter = Cc["@mozilla.org/updates/update-prompt;1"].
                     createInstance(Ci.nsIUpdatePrompt);
      prompter.showUpdateDownloaded(this._update, true);
    }

    if (state == STATE_PENDING || state == STATE_PENDING_SVC) {
      
      Cc["@mozilla.org/updates/update-service;1"].
        getService(Ci.nsIApplicationUpdateService).
        applyUpdateInBackground(this._update);
    }

    
    this._update = null;
  },

  


  getInterface: function Downloader_getInterface(iid) {
    
    
    if (iid.equals(Ci.nsIAuthPrompt)) {
      var prompt = Cc["@mozilla.org/network/default-auth-prompt;1"].
                   createInstance();
      return prompt.QueryInterface(iid);
    }
    throw Components.results.NS_NOINTERFACE;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIRequestObserver,
                                         Ci.nsIProgressEventSink,
                                         Ci.nsIInterfaceRequestor])
};








function UpdatePrompt() {
}
UpdatePrompt.prototype = {
  


  checkForUpdates: function UP_checkForUpdates() {
    if (this._getAltUpdateWindow())
      return;

    this._showUI(null, URI_UPDATE_PROMPT_DIALOG, null, UPDATE_WINDOW_NAME,
                 null, null);
  },

  


  showUpdateAvailable: function UP_showUpdateAvailable(update) {
    if (getPref("getBoolPref", PREF_APP_UPDATE_SILENT, false) ||
        this._getUpdateWindow() || this._getAltUpdateWindow())
      return;

    var stringsPrefix = "updateAvailable_" + update.type + ".";
    var title = gUpdateBundle.formatStringFromName(stringsPrefix + "title",
                                                   [update.name], 1);
    var text = gUpdateBundle.GetStringFromName(stringsPrefix + "text");
    var imageUrl = "";
    this._showUnobtrusiveUI(null, URI_UPDATE_PROMPT_DIALOG, null,
                           UPDATE_WINDOW_NAME, "updatesavailable", update,
                           title, text, imageUrl);
  },

  


  showUpdateDownloaded: function UP_showUpdateDownloaded(update, background) {
    if (this._getAltUpdateWindow())
      return;

    if (background) {
      if (getPref("getBoolPref", PREF_APP_UPDATE_SILENT, false))
        return;

      var stringsPrefix = "updateDownloaded_" + update.type + ".";
      var title = gUpdateBundle.formatStringFromName(stringsPrefix + "title",
                                                     [update.name], 1);
      var text = gUpdateBundle.GetStringFromName(stringsPrefix + "text");
      var imageUrl = "";
      this._showUnobtrusiveUI(null, URI_UPDATE_PROMPT_DIALOG, null,
                              UPDATE_WINDOW_NAME, "finishedBackground", update,
                              title, text, imageUrl);
    } else {
      this._showUI(null, URI_UPDATE_PROMPT_DIALOG, null,
                   UPDATE_WINDOW_NAME, "finishedBackground", update);
    }
  },

  


  showUpdateInstalled: function UP_showUpdateInstalled() {
    if (getPref("getBoolPref", PREF_APP_UPDATE_SILENT, false) ||
        !getPref("getBoolPref", PREF_APP_UPDATE_SHOW_INSTALLED_UI, false) ||
        this._getUpdateWindow())
      return;

    var page = "installed";
    var win = this._getUpdateWindow();
    if (win) {
      if (page && "setCurrentPage" in win)
        win.setCurrentPage(page);
      win.focus();
    }
    else {
      var openFeatures = "chrome,centerscreen,dialog=no,resizable=no,titlebar,toolbar=no";
      var arg = Cc["@mozilla.org/supports-string;1"].
                createInstance(Ci.nsISupportsString);
      arg.data = page;
      Services.ww.openWindow(null, URI_UPDATE_PROMPT_DIALOG, null, openFeatures, arg);
    }
  },

  


  showUpdateError: function UP_showUpdateError(update) {
    if (getPref("getBoolPref", PREF_APP_UPDATE_SILENT, false) ||
        this._getAltUpdateWindow())
      return;

    
    if (update.state == STATE_FAILED && update.errorCode == WRITE_ERROR) {
      var title = gUpdateBundle.GetStringFromName("updaterIOErrorTitle");
      var text = gUpdateBundle.formatStringFromName("updaterIOErrorMsg",
                                                    [Services.appinfo.name,
                                                     Services.appinfo.name], 2);
      Services.ww.getNewPrompter(null).alert(title, text);
      return;
    }

    if (update.errorCode == CERT_ATTR_CHECK_FAILED_NO_UPDATE ||
        update.errorCode == CERT_ATTR_CHECK_FAILED_HAS_UPDATE ||
        update.errorCode == BACKGROUNDCHECK_MULTIPLE_FAILURES) {
      this._showUIWhenIdle(null, URI_UPDATE_PROMPT_DIALOG, null,
                           UPDATE_WINDOW_NAME, null, update);
      return;
    }

    this._showUI(null, URI_UPDATE_PROMPT_DIALOG, null, UPDATE_WINDOW_NAME,
                 "errors", update);
  },

  


  showUpdateHistory: function UP_showUpdateHistory(parent) {
    this._showUI(parent, URI_UPDATE_HISTORY_DIALOG, "modal,dialog=yes",
                 "Update:History", null, null);
  },

  


  _getUpdateWindow: function UP__getUpdateWindow() {
    return Services.wm.getMostRecentWindow(UPDATE_WINDOW_NAME);
  },

  




  _getAltUpdateWindow: function UP__getAltUpdateWindow() {
    let windowType = getPref("getCharPref", PREF_APP_UPDATE_ALTWINDOWTYPE, null);
    if (!windowType)
      return null;
    return Services.wm.getMostRecentWindow(windowType);
  },

  




















  _showUnobtrusiveUI: function UP__showUnobUI(parent, uri, features, name, page,
                                              update, title, text, imageUrl) {
    var observer = {
      updatePrompt: this,
      service: null,
      timer: null,
      notify: function () {
        
        this.service.removeObserver(this, "quit-application");
        
        if (this.updatePrompt._getUpdateWindow())
          return;
        this.updatePrompt._showUIWhenIdle(parent, uri, features, name, page, update);
      },
      observe: function (aSubject, aTopic, aData) {
        switch (aTopic) {
          case "alertclickcallback":
            this.updatePrompt._showUI(parent, uri, features, name, page, update);
            
          case "quit-application":
            if (this.timer)
              this.timer.cancel();
            this.service.removeObserver(this, "quit-application");
            break;
        }
      }
    };

    
    
    
    if (page == "updatesavailable") {
      var idleService = Cc["@mozilla.org/widget/idleservice;1"].
                        getService(Ci.nsIIdleService);

      const IDLE_TIME = getPref("getIntPref", PREF_APP_UPDATE_IDLETIME, 60);
      if (idleService.idleTime / 1000 >= IDLE_TIME) {
        this._showUI(parent, uri, features, name, page, update);
        return;
      }
    }

    try {
      var notifier = Cc["@mozilla.org/alerts-service;1"].
                     getService(Ci.nsIAlertsService);
      notifier.showAlertNotification(imageUrl, title, text, true, "", observer);
    }
    catch (e) {
      
      this._showUIWhenIdle(parent, uri, features, name, page, update);
      return;
    }

    observer.service = Services.obs;
    observer.service.addObserver(observer, "quit-application", false);

    
    if (page == "updatesavailable") {
      this._showUIWhenIdle(parent, uri, features, name, page, update);
      return;
    }

    
    var promptWaitTime = getPref("getIntPref", PREF_APP_UPDATE_PROMPTWAITTIME, 43200);
    observer.timer = Cc["@mozilla.org/timer;1"].
                     createInstance(Ci.nsITimer);
    observer.timer.initWithCallback(observer, promptWaitTime * 1000,
                                    observer.timer.TYPE_ONE_SHOT);
  },

  














  _showUIWhenIdle: function UP__showUIWhenIdle(parent, uri, features, name,
                                               page, update) {
    var idleService = Cc["@mozilla.org/widget/idleservice;1"].
                      getService(Ci.nsIIdleService);

    const IDLE_TIME = getPref("getIntPref", PREF_APP_UPDATE_IDLETIME, 60);
    if (idleService.idleTime / 1000 >= IDLE_TIME) {
      this._showUI(parent, uri, features, name, page, update);
    } else {
      var observer = {
        updatePrompt: this,
        observe: function (aSubject, aTopic, aData) {
          switch (aTopic) {
            case "idle":
              
              if (!this.updatePrompt._getUpdateWindow())
                this.updatePrompt._showUI(parent, uri, features, name, page, update);
              
            case "quit-application":
              idleService.removeIdleObserver(this, IDLE_TIME);
              Services.obs.removeObserver(this, "quit-application");
              break;
          }
        }
      };
      idleService.addIdleObserver(observer, IDLE_TIME);
      Services.obs.addObserver(observer, "quit-application", false);
    }
  },

  














  _showUI: function UP__showUI(parent, uri, features, name, page, update) {
    var ary = null;
    if (update) {
      ary = Cc["@mozilla.org/supports-array;1"].
            createInstance(Ci.nsISupportsArray);
      ary.AppendElement(update);
    }

    var win = this._getUpdateWindow();
    if (win) {
      if (page && "setCurrentPage" in win)
        win.setCurrentPage(page);
      win.focus();
    }
    else {
      var openFeatures = "chrome,centerscreen,dialog=no,resizable=no,titlebar,toolbar=no";
      if (features)
        openFeatures += "," + features;
      Services.ww.openWindow(parent, uri, "", openFeatures, ary);
    }
  },

  classDescription: "Update Prompt",
  contractID: "@mozilla.org/updates/update-prompt;1",
  classID: Components.ID("{27ABA825-35B5-4018-9FDD-F99250A0E722}"),
  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdatePrompt])
};

var components = [UpdateService, Checker, UpdatePrompt, UpdateManager];
var NSGetFactory = XPCOMUtils.generateNSGetFactory(components);

#if 0





function STACK(string) {
  dump("*** " + string + "\n");
  stackTrace(arguments.callee.caller.arguments, -1);
}

function stackTraceFunctionFormat(aFunctionName) {
  var classDelimiter = aFunctionName.indexOf("_");
  var className = aFunctionName.substr(0, classDelimiter);
  if (!className)
    className = "<global>";
  var functionName = aFunctionName.substr(classDelimiter + 1, aFunctionName.length);
  if (!functionName)
    functionName = "<anonymous>";
  return className + "::" + functionName;
}

function stackTraceArgumentsFormat(aArguments) {
  arglist = "";
  for (var i = 0; i < aArguments.length; i++) {
    arglist += aArguments[i];
    if (i < aArguments.length - 1)
      arglist += ", ";
  }
  return arglist;
}

function stackTrace(aArguments, aMaxCount) {
  dump("=[STACKTRACE]=====================================================\n");
  dump("*** at: " + stackTraceFunctionFormat(aArguments.callee.name) + "(" +
       stackTraceArgumentsFormat(aArguments) + ")\n");
  var temp = aArguments.callee.caller;
  var count = 0;
  while (temp) {
    dump("***     " + stackTraceFunctionFormat(temp.name) + "(" +
         stackTraceArgumentsFormat(temp.arguments) + ")\n");

    temp = temp.arguments.callee.caller;
    if (aMaxCount > 0 && ++count == aMaxCount)
      break;
  }
  dump("==================================================================\n");
}

function dumpFile(file) {
  dump("*** file = " + file.path + ", exists = " + file.exists() + "\n");
}
#endif
