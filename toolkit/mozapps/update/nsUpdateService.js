





const { classes: Cc, interfaces: Ci, results: Cr, utils: Cu } = Components;

Cu.import("resource://gre/modules/XPCOMUtils.jsm", this);
Cu.import("resource://gre/modules/FileUtils.jsm", this);
Cu.import("resource://gre/modules/AddonManager.jsm", this);
Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/ctypes.jsm", this);
Cu.import("resource://gre/modules/UpdateTelemetry.jsm", this);
Cu.import("resource://gre/modules/AppConstants.jsm", this);

const UPDATESERVICE_CID = Components.ID("{B3C290A6-3943-4B89-8BBE-C01EB7B3B311}");
const UPDATESERVICE_CONTRACTID = "@mozilla.org/updates/update-service;1";

const PREF_APP_UPDATE_ALTWINDOWTYPE       = "app.update.altwindowtype";
const PREF_APP_UPDATE_AUTO                = "app.update.auto";
const PREF_APP_UPDATE_BACKGROUND_INTERVAL = "app.update.download.backgroundInterval";
const PREF_APP_UPDATE_BACKGROUNDERRORS    = "app.update.backgroundErrors";
const PREF_APP_UPDATE_BACKGROUNDMAXERRORS = "app.update.backgroundMaxErrors";
const PREF_APP_UPDATE_CANCELATIONS        = "app.update.cancelations";
const PREF_APP_UPDATE_CERTS_BRANCH        = "app.update.certs.";
const PREF_APP_UPDATE_CERT_CHECKATTRS     = "app.update.cert.checkAttributes";
const PREF_APP_UPDATE_CERT_ERRORS         = "app.update.cert.errors";
const PREF_APP_UPDATE_CERT_MAXERRORS      = "app.update.cert.maxErrors";
const PREF_APP_UPDATE_CERT_REQUIREBUILTIN = "app.update.cert.requireBuiltIn";
const PREF_APP_UPDATE_CUSTOM              = "app.update.custom";
const PREF_APP_UPDATE_ENABLED             = "app.update.enabled";
const PREF_APP_UPDATE_IDLETIME            = "app.update.idletime";
const PREF_APP_UPDATE_INCOMPATIBLE_MODE   = "app.update.incompatible.mode";
const PREF_APP_UPDATE_INTERVAL            = "app.update.interval";
const PREF_APP_UPDATE_LOG                 = "app.update.log";
const PREF_APP_UPDATE_MODE                = "app.update.mode";
const PREF_APP_UPDATE_NEVER_BRANCH        = "app.update.never.";
const PREF_APP_UPDATE_NOTIFIEDUNSUPPORTED = "app.update.notifiedUnsupported";
const PREF_APP_UPDATE_POSTUPDATE          = "app.update.postupdate";
const PREF_APP_UPDATE_PROMPTWAITTIME      = "app.update.promptWaitTime";
const PREF_APP_UPDATE_SHOW_INSTALLED_UI   = "app.update.showInstalledUI";
const PREF_APP_UPDATE_SILENT              = "app.update.silent";
const PREF_APP_UPDATE_STAGING_ENABLED     = "app.update.staging.enabled";
const PREF_APP_UPDATE_URL                 = "app.update.url";
const PREF_APP_UPDATE_URL_DETAILS         = "app.update.url.details";
const PREF_APP_UPDATE_URL_OVERRIDE        = "app.update.url.override";
const PREF_APP_UPDATE_SERVICE_ENABLED     = "app.update.service.enabled";
const PREF_APP_UPDATE_SERVICE_ERRORS      = "app.update.service.errors";
const PREF_APP_UPDATE_SERVICE_MAX_ERRORS  = "app.update.service.maxErrors";
const PREF_APP_UPDATE_SOCKET_ERRORS       = "app.update.socket.maxErrors";
const PREF_APP_UPDATE_RETRY_TIMEOUT       = "app.update.socket.retryTimeout";

const PREF_APP_DISTRIBUTION               = "distribution.id";
const PREF_APP_DISTRIBUTION_VERSION       = "distribution.version";

const PREF_APP_B2G_VERSION                = "b2g.version";

const PREF_EM_HOTFIX_ID                   = "extensions.hotfix.id";

const URI_UPDATE_PROMPT_DIALOG  = "chrome://mozapps/content/update/updates.xul";
const URI_UPDATE_HISTORY_DIALOG = "chrome://mozapps/content/update/history.xul";
const URI_BRAND_PROPERTIES      = "chrome://branding/locale/brand.properties";
const URI_UPDATES_PROPERTIES    = "chrome://mozapps/locale/update/updates.properties";
const URI_UPDATE_NS             = "http://www.mozilla.org/2005/app-update";

const KEY_GRED            = "GreD";
const KEY_UPDROOT         = "UpdRootD";
const KEY_EXECUTABLE      = "XREExeF";

const KEY_UPDATE_ARCHIVE_DIR = "UpdArchD";

const DIR_UPDATED         = "updated";
const DIR_UPDATED_APP     = "Updated.app";
const DIR_UPDATES         = "updates";

const FILE_UPDATE_STATUS  = "update.status";
const FILE_UPDATE_VERSION = "update.version";
const FILE_UPDATE_ARCHIVE = "update.mar";
const FILE_UPDATE_LINK    = "update.link";
const FILE_UPDATE_LOG     = "update.log";
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
const STATE_APPLIED_OS      = "applied-os";
const STATE_APPLIED_SVC     = "applied-service";
const STATE_SUCCEEDED       = "succeeded";
const STATE_DOWNLOAD_FAILED = "download-failed";
const STATE_FAILED          = "failed";


const WRITE_ERROR                          = 7;
const ELEVATION_CANCELED                   = 9;
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
const WRITE_ERROR_ACCESS_DENIED            = 35;
const WRITE_ERROR_CALLBACK_APP             = 37;
const FILESYSTEM_MOUNT_READWRITE_ERROR     = 43;
const SERVICE_COULD_NOT_COPY_UPDATER       = 49;
const SERVICE_STILL_APPLYING_TERMINATED    = 50;
const SERVICE_STILL_APPLYING_NO_EXIT_CODE  = 51;
const WRITE_ERROR_FILE_COPY                = 61;
const WRITE_ERROR_DELETE_FILE              = 62;
const WRITE_ERROR_OPEN_PATCH_FILE          = 63;
const WRITE_ERROR_PATCH_FILE               = 64;
const WRITE_ERROR_APPLY_DIR_PATH           = 65;
const WRITE_ERROR_CALLBACK_PATH            = 66;
const WRITE_ERROR_FILE_ACCESS_DENIED       = 67;
const WRITE_ERROR_DIR_ACCESS_DENIED        = 68;
const WRITE_ERROR_DELETE_BACKUP            = 69;
const WRITE_ERROR_EXTRACT                  = 70;


const WRITE_ERRORS = [WRITE_ERROR,
                      WRITE_ERROR_ACCESS_DENIED,
                      WRITE_ERROR_CALLBACK_APP,
                      WRITE_ERROR_FILE_COPY,
                      WRITE_ERROR_DELETE_FILE,
                      WRITE_ERROR_OPEN_PATCH_FILE,
                      WRITE_ERROR_PATCH_FILE,
                      WRITE_ERROR_APPLY_DIR_PATH,
                      WRITE_ERROR_CALLBACK_PATH,
                      WRITE_ERROR_FILE_ACCESS_DENIED,
                      WRITE_ERROR_DIR_ACCESS_DENIED,
                      WRITE_ERROR_DELETE_BACKUP,
                      WRITE_ERROR_EXTRACT];


const SERVICE_ERRORS = [SERVICE_UPDATER_COULD_NOT_BE_STARTED,
                        SERVICE_NOT_ENOUGH_COMMAND_LINE_ARGS,
                        SERVICE_UPDATER_SIGN_ERROR,
                        SERVICE_UPDATER_COMPARE_ERROR,
                        SERVICE_UPDATER_IDENTITY_ERROR,
                        SERVICE_STILL_APPLYING_ON_SUCCESS,
                        SERVICE_STILL_APPLYING_ON_FAILURE,
                        SERVICE_UPDATER_NOT_FIXED_DRIVE,
                        SERVICE_COULD_NOT_LOCK_UPDATER,
                        SERVICE_INSTALLDIR_ERROR,
                        SERVICE_COULD_NOT_COPY_UPDATER,
                        SERVICE_STILL_APPLYING_TERMINATED,
                        SERVICE_STILL_APPLYING_NO_EXIT_CODE];



const FOTA_GENERAL_ERROR                   = 80;
const FOTA_UNKNOWN_ERROR                   = 81;
const FOTA_FILE_OPERATION_ERROR            = 82;
const FOTA_RECOVERY_ERROR                  = 83;

const STAGE_FAIL_FALLBACK                  = 97;
const INVALID_UPDATER_STATE_CODE           = 98;
const INVALID_UPDATER_STATUS_CODE          = 99;


const CERT_ATTR_CHECK_FAILED_NO_UPDATE  = 100;
const CERT_ATTR_CHECK_FAILED_HAS_UPDATE = 101;
const BACKGROUNDCHECK_MULTIPLE_FAILURES = 110;
const NETWORK_ERROR_OFFLINE             = 111;


const HTTP_ERROR_OFFSET                 = 1000;

const DOWNLOAD_CHUNK_SIZE           = 300000; 
const DOWNLOAD_BACKGROUND_INTERVAL  = 600;    
const DOWNLOAD_FOREGROUND_INTERVAL  = 0;

const UPDATE_WINDOW_NAME      = "Update:Wizard";



const DEFAULT_SERVICE_MAX_ERRORS = 10;



const DEFAULT_SOCKET_MAX_ERRORS = 10;


const DEFAULT_UPDATE_RETRY_TIMEOUT = 2000;

var gLocale = null;
var gUpdateMutexHandle = null;


var gSDCardMountLock = null;


XPCOMUtils.defineLazyGetter(this, "gExtStorage", function aus_gExtStorage() {
  if (AppConstants.platform != "gonk") {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  }
  return Services.env.get("EXTERNAL_STORAGE");
});

XPCOMUtils.defineLazyModuleGetter(this, "UpdateChannel",
                                  "resource://gre/modules/UpdateChannel.jsm");

XPCOMUtils.defineLazyGetter(this, "gLogEnabled", function aus_gLogEnabled() {
  return getPref("getBoolPref", PREF_APP_UPDATE_LOG, false);
});

XPCOMUtils.defineLazyGetter(this, "gUpdateBundle", function aus_gUpdateBundle() {
  return Services.strings.createBundle(URI_UPDATES_PROPERTIES);
});


XPCOMUtils.defineLazyGetter(this, "gCertUtils", function aus_gCertUtils() {
  let temp = { };
  Cu.import("resource://gre/modules/CertUtils.jsm", temp);
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

  if (AppConstants.platform == "macosx") {
    
    
    let macutils = Cc["@mozilla.org/xpcom/mac-utils;1"].
                   getService(Ci.nsIMacUtils);

    if (macutils.isUniversalBinary) {
      abi += "-u-" + macutils.architecturesInBinary;
    }
  }
  return abi;
});

XPCOMUtils.defineLazyGetter(this, "gOSVersion", function aus_gOSVersion() {
  let osVersion;
  try {
    osVersion = Services.sysinfo.getProperty("name") + " " +
                Services.sysinfo.getProperty("version");
  }
  catch (e) {
    LOG("gOSVersion - OS Version unknown: updates are not possible.");
  }

  if (osVersion) {
    if (AppConstants.platform == "win") {
      const BYTE = ctypes.uint8_t;
      const WORD = ctypes.uint16_t;
      const DWORD = ctypes.uint32_t;
      const WCHAR = ctypes.char16_t;
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
            let winSystemInfo = SYSTEM_INFO();
            
            winSystemInfo.wProcessorArchitecture = 0xffff;

            GetNativeSystemInfo(winSystemInfo.address());
            switch(winSystemInfo.wProcessorArchitecture) {
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
    }

    try {
      osVersion += " (" + Services.sysinfo.getProperty("secondaryLibrary") + ")";
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






function closeHandle(handle) {
  let lib = ctypes.open("kernel32.dll");
  let CloseHandle = lib.declare("CloseHandle",
                                ctypes.winapi_abi,
                                ctypes.int32_t, 
                                ctypes.void_t.ptr); 
  CloseHandle(handle);
  lib.close();
}










function createMutex(aName, aAllowExisting = true) {
  if (AppConstants.platform != "win") {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  }

  const INITIAL_OWN = 1;
  const ERROR_ALREADY_EXISTS = 0xB7;
  let lib = ctypes.open("kernel32.dll");
  let CreateMutexW = lib.declare("CreateMutexW",
                                 ctypes.winapi_abi,
                                 ctypes.void_t.ptr, 
                                 ctypes.void_t.ptr, 
                                 ctypes.int32_t, 
                                 ctypes.char16_t.ptr); 

  let handle = CreateMutexW(null, INITIAL_OWN, aName);
  let alreadyExists = ctypes.winLastError == ERROR_ALREADY_EXISTS;
  if (handle && !handle.isNull() && !aAllowExisting && alreadyExists) {
    closeHandle(handle);
    handle = null;
  }
  lib.close();

  if (handle && handle.isNull()) {
    handle = null;
  }

  return handle;
}









function getPerInstallationMutexName(aGlobal = true) {
  if (AppConstants.platform != "win") {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  }

  let hasher = Cc["@mozilla.org/security/hash;1"].
               createInstance(Ci.nsICryptoHash);
  hasher.init(hasher.SHA1);

  let exeFile = Services.dirsvc.get(KEY_EXECUTABLE, Ci.nsILocalFile);

  let converter = Cc["@mozilla.org/intl/scriptableunicodeconverter"].
                  createInstance(Ci.nsIScriptableUnicodeConverter);
  converter.charset = "UTF-8";
  var data = converter.convertToByteArray(exeFile.path.toLowerCase());

  hasher.update(data, data.length);
  return (aGlobal ? "Global\\" : "") + "MozillaUpdateMutex-" + hasher.finish(true);
}









function hasUpdateMutex() {
  if (AppConstants.platform != "win") {
    return true;
  }
  if (!gUpdateMutexHandle) {
    gUpdateMutexHandle = createMutex(getPerInstallationMutexName(true), false);
  }
  return !!gUpdateMutexHandle;
}

XPCOMUtils.defineLazyGetter(this, "gCanApplyUpdates", function aus_gCanApplyUpdates() {
  let useService = false;
  if (shouldUseService() && isServiceInstalled()) {
    
    
    LOG("gCanApplyUpdates - bypass the write checks because we'll use the service");
    useService = true;
  }

  if (!useService) {
    try {
      let updateTestFile = getUpdateFile([FILE_PERMS_TEST]);
      LOG("gCanApplyUpdates - testing write access " + updateTestFile.path);
      testWriteAccess(updateTestFile, false);
      if (AppConstants.platform == "macosx") {
        
        let appDirTestFile = getAppBaseDir();
        appDirTestFile.append(FILE_PERMS_TEST);
        LOG("gCanApplyUpdates - testing write access " + appDirTestFile.path);
        if (appDirTestFile.exists()) {
          appDirTestFile.remove(false)
        }
        appDirTestFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
        appDirTestFile.remove(false);
      } else if (AppConstants.platform == "win") {
        
        let windowsVersion = Services.sysinfo.getProperty("version");
        LOG("gCanApplyUpdates - windowsVersion = " + windowsVersion);

      










        let userCanElevate = false;

        if (parseFloat(windowsVersion) >= 6) {
          try {
            
            
            let dir = Services.dirsvc.get(KEY_UPDROOT, Ci.nsIFile);
            
            userCanElevate = Services.appinfo.QueryInterface(Ci.nsIWinAppHelper).
                             userCanElevate;
            LOG("gCanApplyUpdates - on Vista, userCanElevate: " + userCanElevate);
          }
          catch (ex) {
            
            
            
            LOG("gCanApplyUpdates - on Vista, appDir is not under Program Files");
          }
        }

        



















        if (!userCanElevate) {
          
          let appDirTestFile = getAppBaseDir();
          appDirTestFile.append(FILE_PERMS_TEST);
          LOG("gCanApplyUpdates - testing write access " + appDirTestFile.path);
          if (appDirTestFile.exists())
            appDirTestFile.remove(false)
          appDirTestFile.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
          appDirTestFile.remove(false);
        }
      }
    } catch (e) {
       LOG("gCanApplyUpdates - unable to apply updates. Exception: " + e);
      
      return false;
    }
  } 

  LOG("gCanApplyUpdates - able to apply updates");
  return true;
});






function getCanStageUpdates() {
  
  if (!getPref("getBoolPref", PREF_APP_UPDATE_STAGING_ENABLED, false)) {
    LOG("getCanStageUpdates - staging updates is disabled by preference " +
        PREF_APP_UPDATE_STAGING_ENABLED);
    return false;
  }

  
  
  
  if (AppConstants.platform == "win" || AppConstants.platform == "gonk") {
    if (getPref("getBoolPref", PREF_APP_UPDATE_SERVICE_ENABLED, false)) {
      
      
      LOG("getCanStageUpdates - able to stage updates because we'll use the service");
      return true;
    }
  }

  if (!hasUpdateMutex()) {
    LOG("getCanStageUpdates - unable to apply updates because another " +
        "instance of the application is already handling updates for this " +
        "installation.");
    return false;
  }

  





  XPCOMUtils.defineLazyGetter(this, "canStageUpdatesSession", function canStageUpdatesSession() {
    try {
      var updateTestFile = getInstallDirRoot();
      updateTestFile.append(FILE_PERMS_TEST);
      LOG("canStageUpdatesSession - testing write access " +
          updateTestFile.path);
      testWriteAccess(updateTestFile, true);
      if (AppConstants.platform != "macosx") {
        
        
        
        updateTestFile = getInstallDirRoot().parent;
        updateTestFile.append(FILE_PERMS_TEST);
        LOG("canStageUpdatesSession - testing write access " +
            updateTestFile.path);
        updateTestFile.createUnique(Ci.nsILocalFile.DIRECTORY_TYPE,
                                    FileUtils.PERMS_DIRECTORY);
        updateTestFile.remove(false);
      }
    } catch (e) {
       LOG("canStageUpdatesSession - unable to stage updates. Exception: " +
           e);
      
      return false;
    }

    LOG("canStageUpdatesSession - able to stage updates");
    return true;
  });

  return canStageUpdatesSession;
}

XPCOMUtils.defineLazyGetter(this, "gCanCheckForUpdates", function aus_gCanCheckForUpdates() {
  
  
  
  var enabled = getPref("getBoolPref", PREF_APP_UPDATE_ENABLED, true);
  if (!enabled && Services.prefs.prefIsLocked(PREF_APP_UPDATE_ENABLED)) {
    LOG("gCanCheckForUpdates - unable to automatically check for updates, " +
        "the preference is disabled and admistratively locked.");
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
  return FileUtils.getDir(KEY_UPDROOT, pathArray, true);
}









function getUpdateDirNoCreate(pathArray) {
  return FileUtils.getDir(KEY_UPDROOT, pathArray, false);
}






function getAppBaseDir() {
  return Services.dirsvc.get(KEY_EXECUTABLE, Ci.nsIFile).parent;
}








function getInstallDirRoot() {
  let dir = getAppBaseDir();
  if (AppConstants.platform == "macosx") {
    
    dir = dir.parent.parent;
  }
  return dir;
}











function getUpdateFile(pathArray) {
  let file = getUpdateDirCreate(pathArray.slice(0, -1));
  file.append(pathArray[pathArray.length - 1]);
  return file;
}











function getStatusTextFromCode(code, defaultCode) {
  let reason;
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







function getUpdatesDirInApplyToDir() {
  let dir = getAppBaseDir();
  if (AppConstants.platform == "macosx") {
    dir = dir.parent.parent; 
    dir.append(DIR_UPDATED_APP);
    dir.append("Contents");
    dir.append("MacOS");
  } else {
    dir.append(DIR_UPDATED);
  }
  dir.append(DIR_UPDATES);
  if (!dir.exists()) {
    dir.create(Ci.nsILocalFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);
  }
  return dir;
}








function readStatusFile(dir) {
  let statusFile = dir.clone();
  statusFile.append(FILE_UPDATE_STATUS);
  let status = readStringFromFile(statusFile) || STATE_NONE;
  LOG("readStatusFile - status: " + status + ", path: " + statusFile.path);
  return status;
}











function writeStatusFile(dir, state) {
  let statusFile = dir.clone();
  statusFile.append(FILE_UPDATE_STATUS);
  writeStringToFile(statusFile, state);
}
















function writeVersionFile(dir, version) {
  let versionFile = dir.clone();
  versionFile.append(FILE_UPDATE_VERSION);
  writeStringToFile(versionFile, version);
}











function getFileFromUpdateLink(dir) {
  if (AppConstants.platform != "gonk") {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  }
  let linkFile = dir.clone();
  linkFile.append(FILE_UPDATE_LINK);
  let link = readStringFromFile(linkFile);
  LOG("getFileFromUpdateLink linkFile.path: " + linkFile.path + ", link: " + link);
  if (!link) {
    return null;
  }
  let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
  file.initWithPath(link);
  return file;
}










function writeLinkFile(dir, patchFile) {
  if (AppConstants.platform != "gonk") {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  }
  let linkFile = dir.clone();
  linkFile.append(FILE_UPDATE_LINK);
  writeStringToFile(linkFile, patchFile.path);
  if (patchFile.path.indexOf(gExtStorage) == 0) {
    
    
    
    acquireSDCardMountLock();
  }
}







function acquireSDCardMountLock() {
  if (AppConstants.platform != "gonk") {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  }
  let volsvc = Cc["@mozilla.org/telephony/volume-service;1"].
                    getService(Ci.nsIVolumeService);
  if (volsvc) {
    gSDCardMountLock = volsvc.createMountLock("sdcard");
  }
}









function isInterruptedUpdate(status) {
  if (AppConstants.platform != "gonk") {
    throw Cr.NS_ERROR_NOT_IMPLEMENTED;
  }
  return (status == STATE_DOWNLOADING) ||
         (status == STATE_PENDING) ||
         (status == STATE_APPLYING);
}






function releaseSDCardMountLock() {
  if (AppConstants.platform != "gonk") {
    throw Cr.NS_ERROR_UNEXPECTED;
  }
  if (gSDCardMountLock) {
    gSDCardMountLock.unlock();
    gSDCardMountLock = null;
  }
}








function shouldUseService() {
  if (AppConstants.MOZ_MAINTENANCE_SERVICE) {
    return getPref("getBoolPref",
                   PREF_APP_UPDATE_SERVICE_ENABLED, false);
  }
  return false;
}







function isServiceInstalled() {
  if (AppConstants.MOZ_MAINTENANCE_SERVICE && AppConstants.platform == "win") {
    let installed = 0;
    try {
      let wrk = Cc["@mozilla.org/windows-registry-key;1"].
                createInstance(Ci.nsIWindowsRegKey);
      wrk.open(wrk.ROOT_KEY_LOCAL_MACHINE,
               "SOFTWARE\\Mozilla\\MaintenanceService",
               wrk.ACCESS_READ | wrk.WOW64_64);
      installed = wrk.readIntValue("Installed");
      wrk.close();
    } catch(e) {
    }
    installed = installed == 1;  
    LOG("isServiceInstalled = " + installed);
    return installed;
  }
  return false;
}





function cleanUpMozUpdaterDirs() {
  try {
    
    var mozUpdaterDir = getUpdatesDir();
    mozUpdaterDir.append("MozUpdater");
    if (mozUpdaterDir.exists()) {
      LOG("cleanUpMozUpdaterDirs - removing MozUpdater directory");
      mozUpdaterDir.remove(true);
    }
  } catch (e) {
    LOG("cleanUpMozUpdaterDirs - Exception: " + e);
  }

  try {
    var tmpDir = Services.dirsvc.get("TmpD", Ci.nsIFile);

    
    
    
    var mozUpdaterDir1 = tmpDir.clone();
    mozUpdaterDir1.append("MozUpdater-1");
    
    
    if (mozUpdaterDir1.exists()) {
      LOG("cleanUpMozUpdaterDirs - Removing top level tmp MozUpdater-i " +
          "directories");
      let i = 0;
      let dirEntries = tmpDir.directoryEntries;
      while (dirEntries.hasMoreElements() && i < 10) {
        let file = dirEntries.getNext().QueryInterface(Ci.nsILocalFile);
        if (file.leafName.startsWith("MozUpdater-") && file.leafName != "MozUpdater-1") {
          file.remove(true);
          i++;
        }
      }
      
      
      if (i < 10) {
        mozUpdaterDir1.remove(true);
      }
    }
  } catch (e) {
    LOG("cleanUpMozUpdaterDirs - Exception: " + e);
  }
}









function cleanUpUpdatesDir(aBackgroundUpdate) {
  
  try {
    var updateDir = getUpdatesDir();
  } catch (e) {
    return;
  }

  
  let file = updateDir.clone();
  file.append(FILE_UPDATE_LOG);
  if (file.exists()) {
    let dir;
    if (aBackgroundUpdate && getUpdateDirNoCreate([]).equals(getAppBaseDir())) {
      dir = getUpdatesDirInApplyToDir();
    } else {
      dir = updateDir.parent;
    }
    let logFile = dir.clone();
    logFile.append(FILE_LAST_LOG);
    if (logFile.exists()) {
      try {
        logFile.moveTo(dir, FILE_BACKUP_LOG);
      } catch (e) {
        LOG("cleanUpUpdatesDir - failed to rename file " + logFile.path +
            " to " + FILE_BACKUP_LOG);
      }
    }

    try {
      file.moveTo(dir, FILE_LAST_LOG);
    } catch (e) {
      LOG("cleanUpUpdatesDir - failed to rename file " + file.path +
          " to " + FILE_LAST_LOG);
    }
  }

  if (!aBackgroundUpdate) {
    let e = updateDir.directoryEntries;
    while (e.hasMoreElements()) {
      let f = e.getNext().QueryInterface(Ci.nsIFile);
      if (AppConstants.platform == "gonk") {
        if (f.leafName == FILE_UPDATE_LINK) {
          let linkedFile = getFileFromUpdateLink(updateDir);
          if (linkedFile && linkedFile.exists()) {
            linkedFile.remove(false);
          }
        }
      }

      
      
      
      try {
        f.remove(true);
      } catch (e) {
        LOG("cleanUpUpdatesDir - failed to remove file " + f.path);
      }
    }
  }
  if (AppConstants.platform == "gonk") {
    releaseSDCardMountLock();
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

  for (let res of ['app', 'gre']) {
    var channel = Services.io.newChannel2("resource://" + res + "/" + FILE_UPDATE_LOCALE,
                                          null,
                                          null,
                                          null,      
                                          Services.scriptSecurityManager.getSystemPrincipal(),
                                          null,      
                                          Ci.nsILoadInfo.SEC_NORMAL,
                                          Ci.nsIContentPolicy.TYPE_DATAREQUEST);
    try {
      var inputStream = channel.open();
      gLocale = readStringFromInputStream(inputStream);
    } catch(e) {}
    if (gLocale)
      break;
  }

  if (!gLocale)
    throw Components.Exception(FILE_UPDATE_LOCALE + " file doesn't exist in " +
                               "either the application or GRE directories",
                               Cr.NS_ERROR_FILE_NOT_FOUND);

  LOG("getLocale - getting locale from file: " + channel.originalURI.spec +
      ", locale: " + gLocale);
  return gLocale;
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
  if (text && text[text.length - 1] == "\n") {
    text = text.slice(0, -1);
  }
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
  if (update.errorCode == FOTA_GENERAL_ERROR ||
      update.errorCode == FOTA_FILE_OPERATION_ERROR ||
      update.errorCode == FOTA_RECOVERY_ERROR ||
      update.errorCode == FOTA_UNKNOWN_ERROR) {
    
    
    
    update.statusText = gUpdateBundle.GetStringFromName("statusFailed");

    Cc["@mozilla.org/updates/update-prompt;1"].
      createInstance(Ci.nsIUpdatePrompt).
      showUpdateError(update);
    writeStatusFile(getUpdatesDir(), STATE_FAILED + ": " + errorCode);
    cleanupActiveUpdate();
    return true;
  }

  
  if (WRITE_ERRORS.indexOf(update.errorCode) != -1 ||
      update.errorCode == FILESYSTEM_MOUNT_READWRITE_ERROR) {
    Cc["@mozilla.org/updates/update-prompt;1"].
      createInstance(Ci.nsIUpdatePrompt).
      showUpdateError(update);
    writeStatusFile(getUpdatesDir(), update.state = STATE_PENDING);
    return true;
  }

  if (update.errorCode == ELEVATION_CANCELED) {
    writeStatusFile(getUpdatesDir(), update.state = STATE_PENDING);
    let cancelations = getPref("getIntPref", PREF_APP_UPDATE_CANCELATIONS, 0);
    cancelations++;
    Services.prefs.setIntPref(PREF_APP_UPDATE_CANCELATIONS, cancelations);
    return true;
  }
  if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_SERVICE_ERRORS)) {
    Services.prefs.clearUserPref(PREF_APP_UPDATE_SERVICE_ERRORS);
  }

  
  if (SERVICE_ERRORS.indexOf(update.errorCode) != -1) {
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
    
    
    LOG("handleFallbackToCompleteUpdate - install of partial patch " +
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

function pingStateAndStatusCodes(aUpdate, aStartup, aStatus) {
  let patchType = AUSTLMY.PATCH_UNKNOWN;
  if (aUpdate && aUpdate.selectedPatch && aUpdate.selectedPatch.type) {
    if (aUpdate.selectedPatch.type == "complete") {
      patchType = AUSTLMY.PATCH_COMPLETE;
    } else if (aUpdate.selectedPatch.type == "partial") {
      patchType = AUSTLMY.PATCH_PARTIAL;
    }
  }

  let suffix = patchType + "_" + (aStartup ? AUSTLMY.STARTUP : AUSTLMY.STAGE);
  let stateCode = 0;
  let parts = aStatus.split(":");
  if (parts.length > 0) {
    switch (parts[0]) {
      case STATE_NONE:
        stateCode = 2;
        break;
      case STATE_DOWNLOADING:
        stateCode = 3;
        break;
      case STATE_PENDING:
        stateCode = 4;
        parts[0] = STATE_FAILED;
        parts.push(STAGE_FAIL_FALLBACK);
        break;
      case STATE_PENDING_SVC:
        stateCode = 5;
        break;
      case STATE_APPLYING:
        stateCode = 6;
        break;
      case STATE_APPLIED:
        stateCode = 7;
        break;
      case STATE_APPLIED_OS:
        stateCode = 8;
        break;
      case STATE_APPLIED_SVC:
        stateCode = 9;
        break;
      case STATE_SUCCEEDED:
        stateCode = 10;
        break;
      case STATE_DOWNLOAD_FAILED:
        stateCode = 11;
        break;
      case STATE_FAILED:
        stateCode = 12;
        break;
      default:
        stateCode = 1;
    }

    if (parts.length > 1) {
      let statusErrorCode = INVALID_UPDATER_STATE_CODE;
      if (parts[0] == STATE_FAILED) {
        statusErrorCode = parseInt(parts[1]) || INVALID_UPDATER_STATUS_CODE;
      }
      AUSTLMY.pingStatusErrorCode(suffix, statusErrorCode);
    }
  }
  AUSTLMY.pingStateCode(suffix, stateCode);
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
    }
  }
}
UpdatePatch.prototype = {
  


  serialize: function UpdatePatch_serialize(updates) {
    var patch = updates.createElementNS(URI_UPDATE_NS, "patch");
    patch.setAttribute("type", this.type);
    patch.setAttribute("URL", this.URL);
    
    if (this.finalURL) {
      patch.setAttribute("finalURL", this.finalURL);
    }
    patch.setAttribute("hashFunction", this.hashFunction);
    patch.setAttribute("hashValue", this.hashValue);
    patch.setAttribute("size", this.size);
    if (this.selected) {
      patch.setAttribute("selected", this.selected);
    }
    patch.setAttribute("state", this.state);

    for (var p in this._properties) {
      if (this._properties[p].present) {
        patch.setAttribute(p, this._properties[p].data);
      }
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
        this._properties[name].present) {
      return this._properties[name].data;
    }
    return null;
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
  this.isOSUpdate = false;
  this.showPrompt = false;
  this.showNeverForVersion = false;
  this.unsupported = false;
  this.channel = "default";
  this.promptWaitTime = getPref("getIntPref", PREF_APP_UPDATE_PROMPTWAITTIME, 43200);

  
  
  if (!update) {
    return;
  }

  const ELEMENT_NODE = Ci.nsIDOMNode.ELEMENT_NODE;
  for (var i = 0; i < update.childNodes.length; ++i) {
    var patchElement = update.childNodes.item(i);
    if (patchElement.nodeType != ELEMENT_NODE ||
        patchElement.localName != "patch") {
      continue;
    }

    patchElement.QueryInterface(Ci.nsIDOMElement);
    try {
      var patch = new UpdatePatch(patchElement);
    } catch (e) {
      continue;
    }
    this._patches.push(patch);
  }

  if (this._patches.length == 0 && !update.hasAttribute("unsupported")) {
    throw Cr.NS_ERROR_ILLEGAL_VALUE;
  }

  
  
  if (!update.hasAttribute("appVersion")) {
    if (update.getAttribute("type") == "major") {
      if (update.hasAttribute("detailsURL")) {
        this.billboardURL = update.getAttribute("detailsURL");
        this.showPrompt = true;
        this.showNeverForVersion = true;
      }
    }
  }

  
  
  
  this.installDate = (new Date()).getTime();

  for (var i = 0; i < update.attributes.length; ++i) {
    var attr = update.attributes.item(i);
    attr.QueryInterface(Ci.nsIDOMAttr);
    if (attr.value == "undefined") {
      continue;
    } else if (attr.name == "detailsURL") {
      this._detailsURL = attr.value;
    } else if (attr.name == "extensionVersion") {
      
      
      if (!this.appVersion) {
        this.appVersion = attr.value;
      }
    } else if (attr.name == "installDate" && attr.value) {
      let val = parseInt(attr.value);
      if (val) {
        this.installDate = val;
      }
    } else if (attr.name == "isCompleteUpdate") {
      this.isCompleteUpdate = attr.value == "true";
    } else if (attr.name == "isSecurityUpdate") {
      this.isSecurityUpdate = attr.value == "true";
    } else if (attr.name == "isOSUpdate") {
      this.isOSUpdate = attr.value == "true";
    } else if (attr.name == "showNeverForVersion") {
      this.showNeverForVersion = attr.value == "true";
    } else if (attr.name == "showPrompt") {
      this.showPrompt = attr.value == "true";
    } else if (attr.name == "promptWaitTime") {
      if(!isNaN(attr.value)) {
        this.promptWaitTime = parseInt(attr.value);
      }
    } else if (attr.name == "unsupported") {
      this.unsupported = attr.value == "true";
    } else if (attr.name == "version") {
      
      
      if (!this.displayVersion) {
        this.displayVersion = attr.value;
      }
    } else {
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
      }
    }
  }

  
  
  var name = "";
  if (update.hasAttribute("name")) {
    name = update.getAttribute("name");
  } else {
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
    
    
    
    if (!this.appVersion) {
      return null;
    }
    var update = updates.createElementNS(URI_UPDATE_NS, "update");
    update.setAttribute("appVersion", this.appVersion);
    update.setAttribute("buildID", this.buildID);
    update.setAttribute("channel", this.channel);
    update.setAttribute("displayVersion", this.displayVersion);
    
    update.setAttribute("extensionVersion", this.appVersion);
    update.setAttribute("installDate", this.installDate);
    update.setAttribute("isCompleteUpdate", this.isCompleteUpdate);
    update.setAttribute("isOSUpdate", this.isOSUpdate);
    update.setAttribute("name", this.name);
    update.setAttribute("serviceURL", this.serviceURL);
    update.setAttribute("showNeverForVersion", this.showNeverForVersion);
    update.setAttribute("showPrompt", this.showPrompt);
    update.setAttribute("promptWaitTime", this.promptWaitTime);
    update.setAttribute("type", this.type);
    
    update.setAttribute("version", this.displayVersion);

    
    if (this.billboardURL) {
      update.setAttribute("billboardURL", this.billboardURL);
    }
    if (this.detailsURL) {
      update.setAttribute("detailsURL", this.detailsURL);
    }
    if (this.licenseURL) {
      update.setAttribute("licenseURL", this.licenseURL);
    }
    if (this.platformVersion) {
      update.setAttribute("platformVersion", this.platformVersion);
    }
    if (this.previousAppVersion) {
      update.setAttribute("previousAppVersion", this.previousAppVersion);
    }
    if (this.statusText) {
      update.setAttribute("statusText", this.statusText);
    }
    if (this.unsupported) {
      update.setAttribute("unsupported", this.unsupported);
    }
    updates.documentElement.appendChild(update);

    for (var p in this._properties) {
      if (this._properties[p].present) {
        update.setAttribute(p, this._properties[p].data);
      }
    }

    for (let i = 0; i < this.patchCount; ++i) {
      update.appendChild(this.getPatchAt(i).serialize(updates));
    }

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
    if (name in this._properties && this._properties[name].present) {
      return this._properties[name].data;
    }
    return null;
  },

  QueryInterface: XPCOMUtils.generateQI([Ci.nsIUpdate,
                                         Ci.nsIPropertyBag,
                                         Ci.nsIWritablePropertyBag])
};

const UpdateServiceFactory = {
  _instance: null,
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return this._instance == null ? this._instance = new UpdateService() :
                                    this._instance;
  }
};






function UpdateService() {
  LOG("Creating UpdateService");
  Services.obs.addObserver(this, "xpcom-shutdown", false);
  Services.prefs.addObserver(PREF_APP_UPDATE_LOG, this, false);
  if (AppConstants.platform == "gonk") {
    
    
    
    Services.obs.addObserver(this, "profile-change-net-teardown", false);
  }
}

UpdateService.prototype = {
  



  _downloader: null,

  


  _incompatAddonsCount: 0,

  


  _registeredOnlineObserver: false,

  


  _consecutiveSocketErrors: 0,

  


  _retryTimer: null,

  



  _isNotify: true,

  








  observe: function AUS_observe(subject, topic, data) {
    switch (topic) {
      case "post-update-processing":
        
        this._postUpdateProcessing();
        break;
      case "network:offline-status-changed":
        this._offlineStatusChanged(data);
        break;
      case "nsPref:changed":
        if (data == PREF_APP_UPDATE_LOG) {
          gLogEnabled = getPref("getBoolPref", PREF_APP_UPDATE_LOG, false);
        }
        break;
      case "profile-change-net-teardown": 
      case "xpcom-shutdown":
        Services.obs.removeObserver(this, topic);
        Services.prefs.removeObserver(PREF_APP_UPDATE_LOG, this);

        if (AppConstants.platform == "win" && gUpdateMutexHandle) {
          
          
          
          closeHandle(gUpdateMutexHandle);
        }
        if (this._retryTimer) {
          this._retryTimer.cancel();
        }

        this.pauseDownload();
        
        this._downloader = null;
        break;
    }
  },

  








  





  _postUpdateProcessing: function AUS__postUpdateProcessing() {
    if (!this.canCheckForUpdates) {
      LOG("UpdateService:_postUpdateProcessing - unable to check for " +
          "updates... returning early");
      return;
    }

    if (!this.canApplyUpdates) {
      LOG("UpdateService:_postUpdateProcessing - unable to apply " +
          "updates... returning early");
      
      
      cleanupActiveUpdate();
      return;
    }

    var um = Cc["@mozilla.org/updates/update-manager;1"].
             getService(Ci.nsIUpdateManager);
    var update = um.activeUpdate;
    var status = readStatusFile(getUpdatesDir());
    pingStateAndStatusCodes(update, true, status);
    
    
    if (status == STATE_NONE) {
      LOG("UpdateService:_postUpdateProcessing - no status, no update");
      cleanupActiveUpdate();
      return;
    }

    if (AppConstants.platform == "gonk") {
      
      
      
      
      
      
      
      
      
      
      if (isInterruptedUpdate(status)) {
        LOG("UpdateService:_postUpdateProcessing - interrupted update detected - wait for user consent");
        return;
      }
    }

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

    if (AppConstants.platform == "gonk") {
      
      if (status == STATE_APPLIED && update && update.isOSUpdate) {
        LOG("UpdateService:_postUpdateProcessing - update staged as applied found");
        return;
      }

      if (status == STATE_APPLIED_OS && update && update.isOSUpdate) {
        
        
        let recoveryService = Cc["@mozilla.org/recovery-service;1"].
                              getService(Ci.nsIRecoveryService);
        let fotaStatus = recoveryService.getFotaUpdateStatus();
        switch (fotaStatus) {
          case Ci.nsIRecoveryService.FOTA_UPDATE_SUCCESS:
            status = STATE_SUCCEEDED;
            break;
          case Ci.nsIRecoveryService.FOTA_UPDATE_FAIL:
            status = STATE_FAILED + ": " + FOTA_GENERAL_ERROR;
            break;
          case Ci.nsIRecoveryService.FOTA_UPDATE_UNKNOWN:
          default:
            status = STATE_FAILED + ": " + FOTA_UNKNOWN_ERROR;
            break;
        }
      }
    }

    if (!update) {
      if (status != STATE_SUCCEEDED) {
        LOG("UpdateService:_postUpdateProcessing - previous patch failed " +
            "and no patch available");
        cleanupActiveUpdate();
        return;
      }
      update = new Update(null);
    }

    var prompter = Cc["@mozilla.org/updates/update-prompt;1"].
                   createInstance(Ci.nsIUpdatePrompt);

    update.state = status;

    if (status == STATE_SUCCEEDED) {
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

    
    
    cleanUpMozUpdaterDirs();
  },

  



  _registerOnlineObserver: function AUS__registerOnlineObserver() {
    if (this._registeredOnlineObserver) {
      LOG("UpdateService:_registerOnlineObserver - observer already registered");
      return;
    }

    LOG("UpdateService:_registerOnlineObserver - waiting for the network to " +
        "be online, then forcing another check");

    Services.obs.addObserver(this, "network:offline-status-changed", false);
    this._registeredOnlineObserver = true;
  },

  


  _offlineStatusChanged: function AUS__offlineStatusChanged(status) {
    if (status !== "online") {
      return;
    }

    Services.obs.removeObserver(this, "network:offline-status-changed");
    this._registeredOnlineObserver = false;

    LOG("UpdateService:_offlineStatusChanged - network is online, forcing " +
        "another background check");

    
    this._attemptResume();
  },

  onCheckComplete: function AUS_onCheckComplete(request, updates, updateCount) {
    this._selectAndInstallUpdate(updates);
  },

  onError: function AUS_onError(request, update) {
    LOG("UpdateService:onError - error during background update. error code: " +
        update.errorCode + ", status text: " + update.statusText);

    var maxErrors;
    var errCount;
    if (update.errorCode == NETWORK_ERROR_OFFLINE) {
      
      this._registerOnlineObserver();
      if (this._pingSuffix) {
        AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_OFFLINE);
      }
      return;
    }

    if (update.errorCode == CERT_ATTR_CHECK_FAILED_NO_UPDATE ||
        update.errorCode == CERT_ATTR_CHECK_FAILED_HAS_UPDATE) {
      errCount = getPref("getIntPref", PREF_APP_UPDATE_CERT_ERRORS, 0);
      errCount++;
      Services.prefs.setIntPref(PREF_APP_UPDATE_CERT_ERRORS, errCount);
      maxErrors = getPref("getIntPref", PREF_APP_UPDATE_CERT_MAXERRORS, 5);
    } else {
      update.errorCode = BACKGROUNDCHECK_MULTIPLE_FAILURES;
      errCount = getPref("getIntPref", PREF_APP_UPDATE_BACKGROUNDERRORS, 0);
      errCount++;
      Services.prefs.setIntPref(PREF_APP_UPDATE_BACKGROUNDERRORS, errCount);
      maxErrors = getPref("getIntPref", PREF_APP_UPDATE_BACKGROUNDMAXERRORS,
                          10);
    }

    let checkCode;
    if (errCount >= maxErrors) {
      var prompter = Cc["@mozilla.org/updates/update-prompt;1"].
                     createInstance(Ci.nsIUpdatePrompt);
      prompter.showUpdateError(update);

      switch (update.errorCode) {
        case CERT_ATTR_CHECK_FAILED_NO_UPDATE:
          checkCode = AUSTLMY.CHK_CERT_ATTR_NO_UPDATE_PROMPT;
          break;
        case CERT_ATTR_CHECK_FAILED_HAS_UPDATE:
          checkCode = AUSTLMY.CHK_CERT_ATTR_WITH_UPDATE_PROMPT;
          break;
        default:
          checkCode = AUSTLMY.CHK_GENERAL_ERROR_PROMPT;
          AUSTLMY.pingCheckExError(this._pingSuffix, update.errorCode);
      }
    } else {
      switch (update.errorCode) {
        case CERT_ATTR_CHECK_FAILED_NO_UPDATE:
          checkCode = AUSTLMY.CHK_CERT_ATTR_NO_UPDATE_SILENT;
          break;
        case CERT_ATTR_CHECK_FAILED_HAS_UPDATE:
          checkCode = AUSTLMY.CHK_CERT_ATTR_WITH_UPDATE_SILENT;
          break;
        default:
          checkCode = AUSTLMY.CHK_GENERAL_ERROR_SILENT;
          AUSTLMY.pingCheckExError(this._pingSuffix, update.errorCode);
      }
    }
    AUSTLMY.pingCheckCode(this._pingSuffix, checkCode);
  },

  


  _attemptResume: function AUS_attemptResume() {
    LOG("UpdateService:_attemptResume")
    
    if (this._downloader && this._downloader._patch &&
        this._downloader._patch.state == STATE_DOWNLOADING &&
        this._downloader._update) {
      LOG("UpdateService:_attemptResume - _patch.state: " +
          this._downloader._patch.state);
      
      writeStatusFile(getUpdatesDir(), STATE_DOWNLOADING);
      var status = this.downloadUpdate(this._downloader._update,
                                       this._downloader.background);
      LOG("UpdateService:_attemptResume - downloadUpdate status: " + status);
      if (status == STATE_NONE) {
        cleanupActiveUpdate();
      }
      return;
    }

    this.backgroundChecker.checkForUpdates(this, false);
  },

  




  notify: function AUS_notify(timer) {
    this._checkForBackgroundUpdates(true);
  },

  


  checkForBackgroundUpdates: function AUS_checkForBackgroundUpdates() {
    this._checkForBackgroundUpdates(false);
  },

  
  get _pingSuffix() {
    return this._isNotify ? AUSTLMY.NOTIFY : AUSTLMY.EXTERNAL;
  },

  





  _checkForBackgroundUpdates: function AUS__checkForBackgroundUpdates(isNotify) {
    this._isNotify = isNotify;

    
    
    
    AUSTLMY.pingGeneric("UPDATE_CANNOT_APPLY_" + this._pingSuffix,
                        gCanApplyUpdates);
    
    
    
    AUSTLMY.pingGeneric("UPDATE_CANNOT_STAGE_" + this._pingSuffix,
                        getCanStageUpdates(), true);
    
    
    
    
    
    AUSTLMY.pingLastUpdateTime(this._pingSuffix);
    
    
    
    AUSTLMY.pingBoolPref("UPDATE_NOT_PREF_UPDATE_ENABLED_" + this._pingSuffix,
                         PREF_APP_UPDATE_ENABLED, true, true);
    
    
    
    AUSTLMY.pingBoolPref("UPDATE_NOT_PREF_UPDATE_AUTO_" + this._pingSuffix,
                         PREF_APP_UPDATE_AUTO, true, true);
    
    
    
    AUSTLMY.pingBoolPref("UPDATE_NOT_PREF_UPDATE_STAGING_ENABLED_" +
                         this._pingSuffix,
                         PREF_APP_UPDATE_STAGING_ENABLED, true, true);
    if (AppConstants.platform == "win") {
      
      
      
      AUSTLMY.pingIntPref("UPDATE_PREF_UPDATE_CANCELATIONS_" + this._pingSuffix,
                          PREF_APP_UPDATE_CANCELATIONS, 0, 0);
    }
    if (AppConstants.MOZ_MAINTENANCE_SERVICE) {
      
      
      
      AUSTLMY.pingBoolPref("UPDATE_NOT_PREF_UPDATE_SERVICE_ENABLED_" +
                           this._pingSuffix,
                           PREF_APP_UPDATE_SERVICE_ENABLED, true);
      
      
      
      AUSTLMY.pingIntPref("UPDATE_PREF_SERVICE_ERRORS_" + this._pingSuffix,
                          PREF_APP_UPDATE_SERVICE_ERRORS, 0, 0);
      if (AppConstants.platform == "win") {
        
        
        
        
        
        AUSTLMY.pingServiceInstallStatus(this._pingSuffix, isServiceInstalled());
      }
    }

    let prefType = Services.prefs.getPrefType(PREF_APP_UPDATE_URL_OVERRIDE);
    let overridePrefHasValue = prefType != Ci.nsIPrefBranch.PREF_INVALID;
    
    
    
    AUSTLMY.pingGeneric("UPDATE_HAS_PREF_URL_OVERRIDE_" + this._pingSuffix,
                        overridePrefHasValue, false);

    
    if (this.isDownloading) {
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_IS_DOWNLOADING);
      return;
    }

    if (this._downloader && this._downloader.patchIsStaged) {
      let readState = readStatusFile(getUpdatesDir());
      if (readState == STATE_PENDING || readState == STATE_PENDING_SVC) {
        AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_IS_DOWNLOADED);
      } else {
        AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_IS_STAGED);
      }
      return;
    }

    let validUpdateURL = true;
    try {
      this.backgroundChecker.getUpdateURL(false);
    } catch (e) {
      validUpdateURL = false;
    }
    
    
    if (!gOSVersion) {
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_NO_OS_VERSION);
    } else if (!gABI) {
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_NO_OS_ABI);
    } else if (!validUpdateURL) {
      if (overridePrefHasValue) {
        if (Services.prefs.prefHasUserValue(PREF_APP_UPDATE_URL_OVERRIDE)) {
          AUSTLMY.pingCheckCode(this._pingSuffix,
                                AUSTLMY.CHK_INVALID_USER_OVERRIDE_URL);
        } else {
          AUSTLMY.pingCheckCode(this._pingSuffix,
                                AUSTLMY.CHK_INVALID_DEFAULT_OVERRIDE_URL);
        }
      } else {
        AUSTLMY.pingCheckCode(this._pingSuffix,
                              AUSTLMY.CHK_INVALID_DEFAULT_URL);
      }
    } else if (!getPref("getBoolPref", PREF_APP_UPDATE_ENABLED, true)) {
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_PREF_DISABLED);
    } else if (!hasUpdateMutex()) {
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_NO_MUTEX);
    } else if (!gCanCheckForUpdates) {
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_UNABLE_TO_CHECK);
    } else if (!this.backgroundChecker._enabled) {
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_DISABLED_FOR_SESSION);
    }

    this.backgroundChecker.checkForUpdates(this, false);
  },

  







  selectUpdate: function AUS_selectUpdate(updates) {
    if (updates.length == 0) {
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_NO_UPDATE_FOUND);
      return null;
    }

    
    if (updates.length == 1 && updates[0].unsupported) {
      return updates[0];
    }

    
    var majorUpdate = null;
    var minorUpdate = null;
    var vc = Services.vc;
    let lastCheckCode = AUSTLMY.CHK_NO_COMPAT_UPDATE_FOUND;

    updates.forEach(function(aUpdate) {
      
      
      if (vc.compare(aUpdate.appVersion, Services.appinfo.version) < 0 ||
          vc.compare(aUpdate.appVersion, Services.appinfo.version) == 0 &&
          aUpdate.buildID == Services.appinfo.appBuildID) {
        LOG("UpdateService:selectUpdate - skipping update because the " +
            "update's application version is less than the current " +
            "application version");
        lastCheckCode = AUSTLMY.CHK_UPDATE_PREVIOUS_VERSION;
        return;
      }

      
      
      
      let neverPrefName = PREF_APP_UPDATE_NEVER_BRANCH + aUpdate.appVersion;
      if (aUpdate.showNeverForVersion &&
          getPref("getBoolPref", neverPrefName, false)) {
        LOG("UpdateService:selectUpdate - skipping update because the " +
            "preference " + neverPrefName + " is true");
        lastCheckCode = AUSTLMY.CHK_UPDATE_NEVER_PREF;
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
          lastCheckCode = AUSTLMY.CHK_UPDATE_INVALID_TYPE;
          break;
      }
    });

    var update = minorUpdate || majorUpdate;
    if (!update) {
      AUSTLMY.pingCheckCode(this._pingSuffix, lastCheckCode);
    }

    return update;
  },

  



  _update: null,

  





  _selectAndInstallUpdate: function AUS__selectAndInstallUpdate(updates) {
    
    
    var um = Cc["@mozilla.org/updates/update-manager;1"].
             getService(Ci.nsIUpdateManager);
    if (um.activeUpdate) {
      if (AppConstants.platform == "gonk") {
        
        
        this._showPrompt(um.activeUpdate);
      }
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_HAS_ACTIVEUPDATE);
      return;
    }

    var updateEnabled = getPref("getBoolPref", PREF_APP_UPDATE_ENABLED, true);
    if (!updateEnabled) {
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_PREF_DISABLED);
      LOG("UpdateService:_selectAndInstallUpdate - not prompting because " +
          "update is disabled");
      return;
    }

    var update = this.selectUpdate(updates, updates.length);
    if (!update) {
      return;
    }

    if (update.unsupported) {
      LOG("UpdateService:_selectAndInstallUpdate - update not supported for " +
          "this system");
      if (!getPref("getBoolPref", PREF_APP_UPDATE_NOTIFIEDUNSUPPORTED, false)) {
        LOG("UpdateService:_selectAndInstallUpdate - notifying that the " +
            "update is not supported for this system");
        this._showPrompt(update);
      }
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_UNSUPPORTED);
      return;
    }

    if (!gCanApplyUpdates) {
      LOG("UpdateService:_selectAndInstallUpdate - the user is unable to " +
          "apply updates... prompting");
      this._showPrompt(update);
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_UNABLE_TO_APPLY);
      return;
    }

    

























    if (update.showPrompt) {
      LOG("UpdateService:_selectAndInstallUpdate - prompting because the " +
          "update snippet specified showPrompt");
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_SHOWPROMPT_SNIPPET);
      this._showPrompt(update);
      return;
    }

    if (!getPref("getBoolPref", PREF_APP_UPDATE_AUTO, true)) {
      LOG("UpdateService:_selectAndInstallUpdate - prompting because silent " +
          "install is disabled");
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_SHOWPROMPT_PREF);
      this._showPrompt(update);
      return;
    }

    if (getPref("getIntPref", PREF_APP_UPDATE_MODE, 1) == 0) {
      
      LOG("UpdateService:_selectAndInstallUpdate - add-on compatibility " +
          "check disabled by preference, just download the update");
      var status = this.downloadUpdate(update, true);
      if (status == STATE_NONE)
        cleanupActiveUpdate();
      AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_ADDON_PREF_DISABLED);
      return;
    }

    
    if (update.appVersion &&
        Services.vc.compare(update.appVersion, Services.appinfo.version) != 0) {
      this._update = update;
      this._checkAddonCompatibility();
    }
    else {
      LOG("UpdateService:_selectAndInstallUpdate - add-on compatibility " +
          "check not performed due to the update version being the same as " +
          "the current application version, just download the update");
      let status = this.downloadUpdate(update, true);
      if (status == STATE_NONE) {
        cleanupActiveUpdate();
      }
      AUSTLMY.pingCheckCode(this._pingSuffix,AUSTLMY.CHK_ADDON_SAME_APP_VER);
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
          if (addon.id) {
            errMsg += " Add-on ID: " + addon.id;
          }
          Cu.reportError(errMsg);
          return;
        }

        
        
        
        
        
        
        
        
        
        
        try {
          if (addon.type != "plugin" && addon.id != hotfixID &&
              !addon.appDisabled && !addon.userDisabled &&
              addon.scope != AddonManager.SCOPE_APPLICATION &&
              addon.isCompatible &&
              !addon.isCompatibleWith(self._update.appVersion,
                                      self._update.platformVersion)) {
            self._incompatibleAddons.push(addon);
          }
        } catch (e) {
          Cu.reportError(e);
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
        LOG("UpdateService:_checkAddonCompatibility - no incompatible " +
            "add-ons found, just download the update");
        var status = self.downloadUpdate(self._update, true);
        if (status == STATE_NONE)
          cleanupActiveUpdate();
        self._update = null;
        AUSTLMY.pingCheckCode(self._pingSuffix, AUSTLMY.CHK_ADDON_NO_INCOMPAT);
      }
    });
  },

  
  onCompatibilityUpdateAvailable: function(addon) {
    
    
    for (var i = 0; i < this._incompatibleAddons.length; ++i) {
      if (this._incompatibleAddons[i].id == addon.id) {
        LOG("UpdateService:onCompatibilityUpdateAvailable - found update for " +
            "add-on ID: " + addon.id);
        this._incompatibleAddons.splice(i, 1);
      }
    }
  },

  onUpdateAvailable: function(addon, install) {
    if (getPref("getIntPref", PREF_APP_UPDATE_INCOMPATIBLE_MODE, 0) == 1) {
      return;
    }

    
    
    
    if (Services.blocklist.isAddonBlocklisted(addon, this._update.appVersion,
                                              this._update.platformVersion)) {
      return;
    }

    
    this.onCompatibilityUpdateAvailable(addon);
  },

  onUpdateFinished: function(addon) {
    if (--this._updateCheckCount > 0) {
      return;
    }

    if (this._incompatibleAddons.length > 0 || !gCanApplyUpdates) {
      LOG("UpdateService:onUpdateEnded - prompting because there are " +
          "incompatible add-ons");
      if (this._incompatibleAddons.length > 0) {
        AUSTLMY.pingCheckCode(this._pingSuffix,
                              AUSTLMY.CHK_ADDON_HAVE_INCOMPAT);
      } else {
        AUSTLMY.pingCheckCode(this._pingSuffix, AUSTLMY.CHK_UNABLE_TO_APPLY);
      }
      this._showPrompt(this._update);
    } else {
      LOG("UpdateService:_selectAndInstallUpdate - updates for all " +
          "incompatible add-ons found, just download the update");
      var status = this.downloadUpdate(this._update, true);
      if (status == STATE_NONE)
        cleanupActiveUpdate();
      AUSTLMY.pingCheckCode(this._pingSuffix,
                            AUSTLMY.CHK_ADDON_UPDATES_FOR_INCOMPAT);
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
    return gCanCheckForUpdates && hasUpdateMutex();
  },

  


  get canApplyUpdates() {
    return gCanApplyUpdates && hasUpdateMutex();
  },

  


  get canStageUpdates() {
    return getCanStageUpdates();
  },

  


  get isOtherInstanceHandlingUpdates() {
    return !hasUpdateMutex();
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
    if (AppConstants.platform == "gonk") {
      let um = Cc["@mozilla.org/updates/update-manager;1"].
               getService(Ci.nsIUpdateManager);
      let activeUpdate = um.activeUpdate;
      if (activeUpdate &&
          (activeUpdate.appVersion != update.appVersion ||
           activeUpdate.buildID != update.buildID)) {
        
        
        
        
        LOG("UpdateService:downloadUpdate - removing stale active update.");
        cleanupActiveUpdate();
      }
    }
    
    update.previousAppVersion = Services.appinfo.version;
    this._downloader = new Downloader(background, this);
    return this._downloader.downloadUpdate(update);
  },

  


  pauseDownload: function AUS_pauseDownload() {
    if (this.isDownloading) {
      this._downloader.cancel();
    } else if (this._retryTimer) {
      
      
      this._retryTimer.cancel();
      this._retryTimer = null;
      this._downloader.cancel();
    }
  },

  


  getUpdatesDirectory: getUpdatesDir,

  


  get isDownloading() {
    return this._downloader && this._downloader.isBusy;
  },

  


  applyOsUpdate: function AUS_applyOsUpdate(aUpdate) {
    if (!aUpdate.isOSUpdate || aUpdate.state != STATE_APPLIED) {
      aUpdate.statusText = "fota-state-error";
      throw Cr.NS_ERROR_FAILURE;
    }

    aUpdate.QueryInterface(Ci.nsIWritablePropertyBag);
    let osApplyToDir = aUpdate.getProperty("osApplyToDir");

    if (!osApplyToDir) {
      LOG("UpdateService:applyOsUpdate - Error: osApplyToDir is not defined" +
          "in the nsIUpdate!");
      pingStateAndStatusCodes(aUpdate, false,
                              STATE_FAILED + ": " + FOTA_FILE_OPERATION_ERROR);
      handleUpdateFailure(aUpdate, FOTA_FILE_OPERATION_ERROR);
      return;
    }

    let updateFile = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
    updateFile.initWithPath(osApplyToDir + "/update.zip");
    if (!updateFile.exists()) {
      LOG("UpdateService:applyOsUpdate - Error: OS update is not found at " +
          updateFile.path);
      pingStateAndStatusCodes(aUpdate, false,
                              STATE_FAILED + ": " + FOTA_FILE_OPERATION_ERROR);
      handleUpdateFailure(aUpdate, FOTA_FILE_OPERATION_ERROR);
      return;
    }

    writeStatusFile(getUpdatesDir(), aUpdate.state = STATE_APPLIED_OS);
    LOG("UpdateService:applyOsUpdate - Rebooting into recovery to apply " +
        "FOTA update: " + updateFile.path);
    try {
      let recoveryService = Cc["@mozilla.org/recovery-service;1"]
                            .getService(Ci.nsIRecoveryService);
      recoveryService.installFotaUpdate(updateFile.path);
    } catch (e) {
      LOG("UpdateService:applyOsUpdate - Error: Couldn't reboot into recovery" +
          " to apply FOTA update " + updateFile.path);
      pingStateAndStatusCodes(aUpdate, false,
                              STATE_FAILED + ": " + FOTA_RECOVERY_ERROR);
      writeStatusFile(getUpdatesDir(), aUpdate.state = STATE_APPLIED);
      handleUpdateFailure(aUpdate, FOTA_RECOVERY_ERROR);
    }
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
                                         Ci.nsIUpdateCheckListener,
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
        this._activeUpdate.channel != UpdateChannel.get()) {
      LOG("UpdateManager:get activeUpdate - channel has changed, " +
          "reloading default preferences to workaround bug 802022");
      
      
      let prefSvc = Services.prefs.QueryInterface(Ci.nsIObserver);
      prefSvc.observe(null, "reload-default-prefs", null);
      if (this._activeUpdate.channel != UpdateChannel.get()) {
        
        
        this._activeUpdate = null;
        this.saveUpdates();

        
        cleanUpUpdatesDir();
      }
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
    if (!file.exists()) {
      file.create(Ci.nsILocalFile.NORMAL_FILE_TYPE, FileUtils.PERMS_FILE);
    }
    fos.init(file, modeFlags, FileUtils.PERMS_FILE, 0);

    try {
      var parser = Cc["@mozilla.org/xmlextras/domparser;1"].
                   createInstance(Ci.nsIDOMParser);
      const EMPTY_UPDATES_DOCUMENT = "<?xml version=\"1.0\"?><updates xmlns=\"http://www.mozilla.org/2005/app-update\"></updates>";
      var doc = parser.parseFromString(EMPTY_UPDATES_DOCUMENT, "text/xml");

      for (var i = 0; i < updates.length; ++i) {
        
        
        
        if (updates[i] && updates[i].appVersion) {
          doc.documentElement.appendChild(updates[i].serialize(doc));
        }
      }

      var serializer = Cc["@mozilla.org/xmlextras/xmlserializer;1"].
                       createInstance(Ci.nsIDOMSerializer);
      serializer.serializeToStream(doc.documentElement, fos, null);
    } catch (e) {
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

  


  refreshUpdateStatus: function UM_refreshUpdateStatus() {
    var update = this._activeUpdate;
    if (!update) {
      return;
    }
    var updateSucceeded = true;
    var status = readStatusFile(getUpdatesDir());
    pingStateAndStatusCodes(update, false, status);
    var parts = status.split(":");
    update.state = parts[0];

    if (update.state == STATE_FAILED && parts[1]) {
      updateSucceeded = false;
      if (!handleUpdateFailure(update, parts[1])) {
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

    
    
    if (AppConstants.platform == "gonk") {
      if (update.state == STATE_APPLIED) {
        
        
        
        let prompter = Cc["@mozilla.org/updates/update-prompt;1"].
                       createInstance(Ci.nsIUpdatePrompt);
        prompter.showUpdateDownloaded(update, true);
      } else {
        releaseSDCardMountLock();
      }
      return;
    }
    
    let windowType = getPref("getCharPref", PREF_APP_UPDATE_ALTWINDOWTYPE, null);
    if (Services.wm.getMostRecentWindow(UPDATE_WINDOW_NAME) ||
        windowType && Services.wm.getMostRecentWindow(windowType)) {
      return;
    }

    if (update.state == STATE_APPLIED || update.state == STATE_APPLIED_SVC ||
        update.state == STATE_PENDING || update.state == STATE_PENDING_SVC) {
        
        
      let prompter = Cc["@mozilla.org/updates/update-prompt;1"].
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

    
    let url = getPref("getCharPref", PREF_APP_UPDATE_URL_OVERRIDE, null);

    
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
    if (/%LOCALE%/.test(url)) {
      url = url.replace(/%LOCALE%/g, getLocale());
    }
    url = url.replace(/%CHANNEL%/g, UpdateChannel.get());
    url = url.replace(/%PLATFORM_VERSION%/g, Services.appinfo.platformVersion);
    url = url.replace(/%DISTRIBUTION%/g,
                      getDistributionPrefValue(PREF_APP_DISTRIBUTION));
    url = url.replace(/%DISTRIBUTION_VERSION%/g,
                      getDistributionPrefValue(PREF_APP_DISTRIBUTION_VERSION));
    url = url.replace(/%CUSTOM%/g, getPref("getCharPref", PREF_APP_UPDATE_CUSTOM, ""));
    url = url.replace(/\+/g, "%2B");

    if (AppConstants.platform == "gonk") {
      let sysLibs = {};
      Cu.import("resource://gre/modules/systemlibs.js", sysLibs);
      url = url.replace(/%PRODUCT_MODEL%/g,
                        sysLibs.libcutils.property_get("ro.product.model"));
      url = url.replace(/%PRODUCT_DEVICE%/g,
                        sysLibs.libcutils.property_get("ro.product.device"));
      url = url.replace(/%B2G_VERSION%/g,
                        getPref("getCharPref", PREF_APP_B2G_VERSION, null));
    }

    if (force) {
      url += (url.indexOf("?") != -1 ? "&" : "?") + "force=1";
    }

    LOG("Checker:getUpdateURL - update URL: " + url);
    return url;
  },

  


  checkForUpdates: function UC_checkForUpdates(listener, force) {
    LOG("Checker: checkForUpdates, force: " + force);
    if (!listener)
      throw Cr.NS_ERROR_NULL_POINTER;

    Services.obs.notifyObservers(null, "update-check-start", null);

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
    
    this._request.channel.loadFlags |= Ci.nsIRequest.LOAD_BYPASS_CACHE;
    
    this._request.channel.loadFlags |= Ci.nsIRequest.INHIBIT_CACHING;

    this._request.overrideMimeType("text/xml");
    
    
    
    this._request.setRequestHeader("Cache-Control", "no-cache");
    
    
    this._request.setRequestHeader("Pragma", "no-cache");

    var self = this;
    this._request.addEventListener("error", function(event) { self.onError(event); } ,false);
    this._request.addEventListener("load", function(event) { self.onLoad(event); }, false);

    LOG("Checker:checkForUpdates - sending request to: " + url);
    this._request.send(null);

    this._callback = listener;
  },

  



  get _updates() {
    var updatesElement = this._request.responseXML.documentElement;
    if (!updatesElement) {
      LOG("Checker:_updates get - empty updates document?!");
      return [];
    }

    if (updatesElement.nodeName != "updates") {
      LOG("Checker:_updates get - unexpected node name!");
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
        LOG("Checker:_updates get - invalid <update/>, ignoring...");
        continue;
      }
      update.serviceURL = this.getUpdateURL(this._forced);
      update.channel = UpdateChannel.get();
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

  _isHttpStatusCode: function UC__isHttpStatusCode(status) {
    return status >= 100 && status <= 599;
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
      update.errorCode = status;
      update.statusText = getStatusTextFromCode(status, 404);

      if (this._isHttpStatusCode(status)) {
        update.errorCode = HTTP_ERROR_OFFSET + status;
      }
      if (e.result && e.result == Cr.NS_ERROR_ILLEGAL_VALUE) {
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
    update.errorCode = status;
    update.statusText = getStatusTextFromCode(status, 200);

    if (status == Cr.NS_ERROR_OFFLINE) {
      
      update.errorCode = NETWORK_ERROR_OFFLINE;
    } else if (this._isHttpStatusCode(status)) {
      update.errorCode = HTTP_ERROR_OFFSET + status;
    }

    this._callback.onError(request, update);
    this._request = null;
  },

  


  _enabled: true,
  get enabled() {
    return getPref("getBoolPref", PREF_APP_UPDATE_ENABLED, true) &&
           gCanCheckForUpdates && hasUpdateMutex() && this._enabled;
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










function Downloader(background, updateService) {
  LOG("Creating Downloader");
  this.background = background;
  this.updateService = updateService;
}
Downloader.prototype = {
  


  _patch: null,

  


  _update: null,

  


  _request: null,

  




  isCompleteUpdate: null,

  


  cancel: function Downloader_cancel(cancelError) {
    LOG("Downloader: cancel");
    if (cancelError === undefined) {
      cancelError = Cr.NS_BINDING_ABORTED;
    }
    if (this._request && this._request instanceof Ci.nsIRequest) {
      this._request.cancel(cancelError);
    }
    if (AppConstants.platform == "gonk") {
      releaseSDCardMountLock();
    }
  },

  


  get patchIsStaged() {
    var readState = readStatusFile(getUpdatesDir());
    
    
    
    return readState == STATE_PENDING || readState == STATE_PENDING_SVC ||
           readState == STATE_APPLIED || readState == STATE_APPLIED_SVC;
  },

  



  _verifyDownload: function Downloader__verifyDownload() {
    LOG("Downloader:_verifyDownload called");
    if (!this._request) {
      AUSTLMY.pingDownloadCode(this.isCompleteUpdate,
                               AUSTLMY.DWNLD_ERR_VERIFY_NO_REQUEST);
      return false;
    }

    let destination = this._request.destination;

    
    if (destination.fileSize != this._patch.size) {
      LOG("Downloader:_verifyDownload downloaded size != expected size.");
      AUSTLMY.pingDownloadCode(this.isCompleteUpdate,
                               AUSTLMY.DWNLD_ERR_VERIFY_PATCH_SIZE_NOT_EQUAL);
      return false;
    }

    LOG("Downloader:_verifyDownload downloaded size == expected size.");
    let fileStream = Cc["@mozilla.org/network/file-input-stream;1"].
                     createInstance(Ci.nsIFileInputStream);
    fileStream.init(destination, FileUtils.MODE_RDONLY, FileUtils.PERMS_FILE, 0);

    let digest;
    try {
      let hash = Cc["@mozilla.org/security/hash;1"].
                 createInstance(Ci.nsICryptoHash);
      var hashFunction = Ci.nsICryptoHash[this._patch.hashFunction.toUpperCase()];
      if (hashFunction == undefined) {
        throw Cr.NS_ERROR_UNEXPECTED;
      }
      hash.init(hashFunction);
      hash.updateFromStream(fileStream, -1);
      
      
      
      
      digest = binaryToHex(hash.finish(false));
    } catch (e) {
      LOG("Downloader:_verifyDownload - failed to compute hash of the " +
          "downloaded update archive");
      digest = "";
    }

    fileStream.close();

    if (digest == this._patch.hashValue.toLowerCase()) {
      LOG("Downloader:_verifyDownload hashes match.");
      return true;
    }

    LOG("Downloader:_verifyDownload hashes do not match. ");
    AUSTLMY.pingDownloadCode(this.isCompleteUpdate,
                             AUSTLMY.DWNLD_ERR_VERIFY_NO_HASH_MATCH);
    return false;
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
      if (state == STATE_DOWNLOADING) {
        LOG("Downloader:_selectPatch - resuming download");
        return selectedPatch;
      }

      if (AppConstants.platform == "gonk") {
        if (state == STATE_PENDING || state == STATE_APPLYING) {
          LOG("Downloader:_selectPatch - resuming interrupted apply");
          return selectedPatch;
        }
        if (state == STATE_APPLIED) {
          LOG("Downloader:_selectPatch - already downloaded and staged");
          return null;
        }
      } else if (state == STATE_PENDING || state == STATE_PENDING_SVC) {
        LOG("Downloader:_selectPatch - already downloaded and staged");
        return null;
      }

      if (update && selectedPatch.type == "complete") {
        
        LOG("Downloader:_selectPatch - failed to apply complete patch!");
        writeStatusFile(updateDir, STATE_NONE);
        writeVersionFile(getUpdatesDir(), null);
        return null;
      }

      
      
      useComplete = true;
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

  


  _getUpdateArchiveFile: function Downloader__getUpdateArchiveFile() {
    var updateArchive;
    if (AppConstants.platform == "gonk") {
      try {
        updateArchive = FileUtils.getDir(KEY_UPDATE_ARCHIVE_DIR, [], true);
      } catch (e) {
        return null;
      }
    } else {
      updateArchive = getUpdatesDir().clone();
    }

    updateArchive.append(FILE_UPDATE_ARCHIVE);
    return updateArchive;
  },

  




  downloadUpdate: function Downloader_downloadUpdate(update) {
    LOG("UpdateService:_downloadUpdate");
    if (!update) {
      AUSTLMY.pingDownloadCode(undefined, AUSTLMY.DWNLD_ERR_NO_UPDATE);
      throw Cr.NS_ERROR_NULL_POINTER;
    }

    var updateDir = getUpdatesDir();

    this._update = update;

    
    
    this._patch = this._selectPatch(update, updateDir);
    if (!this._patch) {
      LOG("Downloader:downloadUpdate - no patch to download");
      AUSTLMY.pingDownloadCode(undefined, AUSTLMY.DWNLD_ERR_NO_UPDATE_PATCH);
      return readStatusFile(updateDir);
    }
    this.isCompleteUpdate = this._patch.type == "complete";

    let patchFile = null;

    
    let status = STATE_NONE;
    if (AppConstants.platform == "gonk") {
      status = readStatusFile(updateDir);
      if (isInterruptedUpdate(status)) {
        LOG("Downloader:downloadUpdate - interruptted update");
        
        
        
        patchFile = getFileFromUpdateLink(updateDir);
        if (!patchFile) {
          
          
          patchFile = updateDir.clone();
          patchFile.append(FILE_UPDATE_ARCHIVE);
        }
        if (patchFile.exists()) {
          LOG("Downloader:downloadUpdate - resuming with patchFile " + patchFile.path);
          if (patchFile.fileSize == this._patch.size) {
            LOG("Downloader:downloadUpdate - patchFile appears to be fully downloaded");
            
            status = STATE_PENDING;
          }
        } else {
          LOG("Downloader:downloadUpdate - patchFile " + patchFile.path +
              " doesn't exist - performing full download");
          
          
          patchFile = null;
        }
        if (patchFile && status != STATE_DOWNLOADING) {
          
          

          writeStatusFile(updateDir, STATE_PENDING);

          
          
          

          this._downloadTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
          this._downloadTimer.initWithCallback(function() {
            this._downloadTimer = null;
            
            
            this._request = {destination: patchFile};
            this.onStopRequest(this._request, null, Cr.NS_OK);
          }.bind(this), 0, Ci.nsITimer.TYPE_ONE_SHOT);

          
          
          
          return STATE_DOWNLOADING;
        }
      }
    }

    if (!patchFile) {
      
      patchFile = this._getUpdateArchiveFile();
    }
    if (!patchFile) {
      AUSTLMY.pingDownloadCode(this.isCompleteUpdate,
                               AUSTLMY.DWNLD_ERR_NO_PATCH_FILE);
      return STATE_NONE;
    }

    if (AppConstants.platform == "gonk") {
      if (patchFile.path.indexOf(updateDir.path) != 0) {
        
        
        writeLinkFile(updateDir, patchFile);

        if (!isInterruptedUpdate(status) && patchFile.exists()) {
          
          patchFile.remove(false);
        }
      }
    }

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

    if (progress > this._patch.size) {
      LOG("Downloader:onProgress - progress: " + progress +
          " is higher than patch size: " + this._patch.size);
      
      
      
      AUSTLMY.pingDownloadCode(this.isCompleteUpdate,
                               AUSTLMY.DWNLD_ERR_PATCH_SIZE_LARGER);
      this.cancel(Cr.NS_ERROR_UNEXPECTED);
      return;
    }

    if (maxProgress != this._patch.size) {
      LOG("Downloader:onProgress - maxProgress: " + maxProgress +
          " is not equal to expectd patch size: " + this._patch.size);
      
      
      
      AUSTLMY.pingDownloadCode(this.isCompleteUpdate,
                               AUSTLMY.DWNLD_ERR_PATCH_SIZE_NOT_EQUAL);
      this.cancel(Cr.NS_ERROR_UNEXPECTED);
      return;
    }

    var listeners = this._listeners.concat();
    var listenerCount = listeners.length;
    for (var i = 0; i < listenerCount; ++i) {
      var listener = listeners[i];
      if (listener instanceof Ci.nsIProgressEventSink)
        listener.onProgress(request, context, progress, maxProgress);
    }
    this.updateService._consecutiveSocketErrors = 0;
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
    var shouldRegisterOnlineObserver = false;
    var shouldRetrySoon = false;
    var deleteActiveUpdate = false;
    var retryTimeout = getPref("getIntPref", PREF_APP_UPDATE_RETRY_TIMEOUT,
                               DEFAULT_UPDATE_RETRY_TIMEOUT);
    var maxFail = getPref("getIntPref", PREF_APP_UPDATE_SOCKET_ERRORS,
                          DEFAULT_SOCKET_MAX_ERRORS);
    LOG("Downloader:onStopRequest - status: " + status + ", " +
        "current fail: " + this.updateService._consecutiveSocketErrors + ", " +
        "max fail: " + maxFail + ", " + "retryTimeout: " + retryTimeout);
    if (Components.isSuccessCode(status)) {
      if (this._verifyDownload()) {
        state = shouldUseService() ? STATE_PENDING_SVC : STATE_PENDING;
        if (this.background) {
          shouldShowPrompt = !getCanStageUpdates();
        }
        AUSTLMY.pingDownloadCode(this.isCompleteUpdate, AUSTLMY.DWNLD_SUCCESS);

        
        writeStatusFile(getUpdatesDir(), state);
        writeVersionFile(getUpdatesDir(), this._update.appVersion);
        this._update.installDate = (new Date()).getTime();
        this._update.statusText = gUpdateBundle.GetStringFromName("installPending");
      }
      else {
        LOG("Downloader:onStopRequest - download verification failed");
        state = STATE_DOWNLOAD_FAILED;
        status = Cr.NS_ERROR_CORRUPTED_CONTENT;

        
        const vfCode = "verification_failed";
        var message = getStatusTextFromCode(vfCode, vfCode);
        this._update.statusText = message;

        if (this._update.isCompleteUpdate || this._update.patchCount != 2)
          deleteActiveUpdate = true;

        
        cleanUpUpdatesDir();
      }
    } else {
      if (status == Cr.NS_ERROR_OFFLINE) {
        
        
        
        
        LOG("Downloader:onStopRequest - offline, register online observer: true");
        AUSTLMY.pingDownloadCode(this.isCompleteUpdate,
                                 AUSTLMY.DWNLD_RETRY_OFFLINE);
        shouldRegisterOnlineObserver = true;
        deleteActiveUpdate = false;
      
      
      
      
      } else if ((status == Cr.NS_ERROR_NET_TIMEOUT ||
                  status == Cr.NS_ERROR_CONNECTION_REFUSED ||
                  status == Cr.NS_ERROR_NET_RESET) &&
                 this.updateService._consecutiveSocketErrors < maxFail) {
        LOG("Downloader:onStopRequest - socket error, shouldRetrySoon: true");
        let dwnldCode = AUSTLMY.DWNLD_RETRY_CONNECTION_REFUSED;
        if (status == Cr.NS_ERROR_NET_TIMEOUT) {
          dwnldCode = AUSTLMY.DWNLD_RETRY_NET_TIMEOUT;
        } else if (status == Cr.NS_ERROR_NET_RESET) {
          dwnldCode = AUSTLMY.DWNLD_RETRY_NET_RESET;
        }
        AUSTLMY.pingDownloadCode(this.isCompleteUpdate, dwnldCode);
        shouldRetrySoon = true;
        deleteActiveUpdate = false;
      } else if (status != Cr.NS_BINDING_ABORTED &&
                 status != Cr.NS_ERROR_ABORT &&
                 status != Cr.NS_ERROR_DOCUMENT_NOT_CACHED) {
        LOG("Downloader:onStopRequest - non-verification failure");
        let dwnldCode = AUSTLMY.DWNLD_ERR_DOCUMENT_NOT_CACHED;
        if (status == Cr.NS_BINDING_ABORTED) {
          dwnldCode = AUSTLMY.DWNLD_ERR_BINDING_ABORTED;
        } else if (status == Cr.NS_ERROR_ABORT) {
          dwnldCode = AUSTLMY.DWNLD_ERR_ABORT;
        }
        AUSTLMY.pingDownloadCode(this.isCompleteUpdate, dwnldCode);

        
        state = STATE_DOWNLOAD_FAILED;

        
        

        this._update.statusText = getStatusTextFromCode(status,
                                                        Cr.NS_BINDING_FAILED);

        if (AppConstants.platform == "gonk") {
          
          
          
          this._update.selectedPatch.selected = false;
        }

        
        cleanUpUpdatesDir();

        deleteActiveUpdate = true;
      }
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
      if (um.activeUpdate) {
        um.activeUpdate.state = state;
      }
    }
    um.saveUpdates();

    
    
    if (!shouldRetrySoon && !shouldRegisterOnlineObserver) {
      var listeners = this._listeners.concat();
      var listenerCount = listeners.length;
      for (var i = 0; i < listenerCount; ++i) {
        listeners[i].onStopRequest(request, context, status);
      }
    }

    this._request = null;

    if (state == STATE_DOWNLOAD_FAILED) {
      var allFailed = true;
      
      if (!this._update.isCompleteUpdate && this._update.patchCount == 2) {
        LOG("Downloader:onStopRequest - verification of patch failed, " +
            "downloading complete update patch");
        this._update.isCompleteUpdate = true;
        let updateStatus = this.downloadUpdate(this._update);

        if (updateStatus == STATE_NONE) {
          cleanupActiveUpdate();
        } else {
          allFailed = false;
        }
      }

      if (allFailed) {
        LOG("Downloader:onStopRequest - all update patch downloads failed");
        
        
        
        
        if (!Services.wm.getMostRecentWindow(UPDATE_WINDOW_NAME)) {
          this._update.QueryInterface(Ci.nsIWritablePropertyBag);
          if (this._update.getProperty("foregroundDownload") == "true") {
            var prompter = Cc["@mozilla.org/updates/update-prompt;1"].
                           createInstance(Ci.nsIUpdatePrompt);
            prompter.showUpdateError(this._update);
          }
        }

        if (AppConstants.platform == "gonk") {
          
          let prompter = Cc["@mozilla.org/updates/update-prompt;1"].
                         createInstance(Ci.nsIUpdatePrompt);
          prompter.showUpdateError(this._update);
        }

        
        this._update = null;
      }
      
      return;
    }

    if (state == STATE_PENDING || state == STATE_PENDING_SVC) {
      if (getCanStageUpdates()) {
        LOG("Downloader:onStopRequest - attempting to stage update: " +
            this._update.name);

        
        try {
          Cc["@mozilla.org/updates/update-processor;1"].
            createInstance(Ci.nsIUpdateProcessor).
            processUpdate(this._update);
        } catch (e) {
          
          
          LOG("Downloader:onStopRequest - failed to stage update. Exception: " +
              e);
          if (this.background) {
            shouldShowPrompt = true;
          }
        }
      }
    }

    
    
    if (shouldShowPrompt) {
      
      
      
      let prompter = Cc["@mozilla.org/updates/update-prompt;1"].
                     createInstance(Ci.nsIUpdatePrompt);
      prompter.showUpdateDownloaded(this._update, true);
    }

    if (shouldRegisterOnlineObserver) {
      LOG("Downloader:onStopRequest - Registering online observer");
      this.updateService._registerOnlineObserver();
    } else if (shouldRetrySoon) {
      LOG("Downloader:onStopRequest - Retrying soon");
      this.updateService._consecutiveSocketErrors++;
      if (this.updateService._retryTimer) {
        this.updateService._retryTimer.cancel();
      }
      this.updateService._retryTimer = Cc["@mozilla.org/timer;1"].createInstance(Ci.nsITimer);
      this.updateService._retryTimer.initWithCallback(function() {
        this._attemptResume();
      }.bind(this.updateService), retryTimeout, Ci.nsITimer.TYPE_ONE_SHOT);
    } else {
      
      this._update = null;
    }
  },

  


  getInterface: function Downloader_getInterface(iid) {
    
    
    if (iid.equals(Ci.nsIAuthPrompt)) {
      var prompt = Cc["@mozilla.org/network/default-auth-prompt;1"].
                   createInstance();
      return prompt.QueryInterface(iid);
    }
    throw Cr.NS_NOINTERFACE;
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

    
    
    if (update.state == STATE_FAILED &&
        (WRITE_ERRORS.indexOf(update.errorCode) != -1 ||
         update.errorCode == FILESYSTEM_MOUNT_READWRITE_ERROR ||
         update.errorCode == FOTA_GENERAL_ERROR ||
         update.errorCode == FOTA_FILE_OPERATION_ERROR ||
         update.errorCode == FOTA_RECOVERY_ERROR ||
         update.errorCode == FOTA_UNKNOWN_ERROR)) {
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

    
    
    observer.timer = Cc["@mozilla.org/timer;1"].
                     createInstance(Ci.nsITimer);
    observer.timer.initWithCallback(observer, update.promptWaitTime * 1000,
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
this.NSGetFactory = XPCOMUtils.generateNSGetFactory(components);
