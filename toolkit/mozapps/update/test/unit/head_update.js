







































const AUS_Cc = Components.classes;
const AUS_Ci = Components.interfaces;
const AUS_Cr = Components.results;

const NS_APP_PROFILE_DIR_STARTUP   = "ProfDS";
const NS_APP_USER_PROFILE_50_DIR   = "ProfD";
const NS_GRE_DIR                   = "GreD";
const NS_XPCOM_CURRENT_PROCESS_DIR = "XCurProcD"
const XRE_UPDATE_ROOT_DIR          = "UpdRootD";

const PREF_APP_UPDATE_URL_OVERRIDE      = "app.update.url.override";
const PREF_APP_UPDATE_SHOW_INSTALLED_UI = "app.update.showInstalledUI";

const URI_UPDATES_PROPERTIES = "chrome://mozapps/locale/update/updates.properties";
const gUpdateBundle = AUS_Cc["@mozilla.org/intl/stringbundle;1"].
                      getService(AUS_Ci.nsIStringBundleService).
                      createBundle(URI_UPDATES_PROPERTIES);

const STATE_NONE            = "null";
const STATE_DOWNLOADING     = "downloading";
const STATE_PENDING         = "pending";
const STATE_APPLYING        = "applying";
const STATE_SUCCEEDED       = "succeeded";
const STATE_DOWNLOAD_FAILED = "download-failed";
const STATE_FAILED          = "failed";

const FILE_UPDATES_DB     = "updates.xml";
const FILE_UPDATE_ACTIVE  = "active-update.xml";

const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;

const PERMS_FILE      = 0644;
const PERMS_DIRECTORY = 0755;

const URL_HOST = "http://localhost:4444/"
const DIR_DATA = "data"

var gDirSvc = AUS_Cc["@mozilla.org/file/directory_service;1"].
              getService(AUS_Ci.nsIProperties);

var gAUS;
var gUpdateChecker;
var gUpdateManager;

var gTestserver;

var gXHR;
var gXHRCallback;

var gCheckFunc;
var gResponseBody;
var gResponseStatusCode = 200;
var gRequestURL;
var gUpdateCount;
var gUpdates;
var gStatusCode;
var gStatusText;


function getPrefBranch() {
  return AUS_Cc["@mozilla.org/preferences;1"].getService(AUS_Ci.nsIPrefBranch);
}







function cleanUp() {
  
  
  
  
  
  if (gAUS)
    gAUS.observe(null, "xpcom-shutdown", "");

  removeUpdateDirsAndFiles();
  gDirSvc.unregisterProvider(gDirProvider);

  if (gXHR) {
    gXHRCallback     = null;

    gXHR.responseXML = null;
    
    gXHR.onerror     = null;
    gXHR.onload      = null;
    gXHR.onprogress  = null;

    gXHR             = null;
  }

  gUpdateManager = null;
  gUpdateChecker = null;
  gAUS           = null;
  gTestserver    = null;
}




function setDefaultPrefs() {
  var pb = getPrefBranch();
  
  
  pb.setBoolPref(PREF_APP_UPDATE_SHOW_INSTALLED_UI, false);
  
  pb.setBoolPref("app.update.log.all", true);
  
  pb.setBoolPref("app.update.enabled", false);
  pb.setBoolPref("extensions.blocklist.enabled", false);
  pb.setBoolPref("extensions.update.enabled", false);
  pb.setBoolPref("browser.search.update", false);
  pb.setBoolPref("browser.microsummary.updateGenerators", false);
}





function startAUS() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1.0", "2.0");
  setDefaultPrefs();
  gAUS = AUS_Cc["@mozilla.org/updates/update-service;1"].
         getService(AUS_Ci.nsIApplicationUpdateService).
         QueryInterface(AUS_Ci.nsIObserver);
  var os = AUS_Cc["@mozilla.org/observer-service;1"].
           getService(AUS_Ci.nsIObserverService);
  os.notifyObservers(null, "profile-after-change", null);
  os.notifyObservers(null, "final-ui-startup", null);
}


function startUpdateChecker() {
  gUpdateChecker = AUS_Cc["@mozilla.org/updates/update-checker;1"].
                   createInstance(AUS_Ci.nsIUpdateChecker);
}


function startUpdateManager() {
  gUpdateManager = AUS_Cc["@mozilla.org/updates/update-manager;1"].
                   getService(AUS_Ci.nsIUpdateManager);
}







function getRemoteUpdatesXMLString(aUpdates) {
  return "<?xml version=\"1.0\"?>\n" +
         "<updates>\n" +
           aUpdates +
         "</updates>\n";
}







function getRemoteUpdateString(aPatches, aName, aType, aVersion,
                               aPlatformVersion, aExtensionVersion, aBuildID,
                               aLicenseURL, aDetailsURL) {
  return  getUpdateString(aName, aType, aVersion, aPlatformVersion,
                         aExtensionVersion, aBuildID, aLicenseURL,
                         aDetailsURL) + ">\n" +
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





















function getLocalUpdateString(aPatches, aName, aType, aVersion, aPlatformVersion,
                              aExtensionVersion, aBuildID, aLicenseURL,
                              aDetailsURL, aServiceURL, aInstallDate, aStatusText,
                              aIsCompleteUpdate, aChannel, aForegroundDownload) {
  var serviceURL = aServiceURL ? aServiceURL : "http://dummyservice/";
  var installDate = aInstallDate ? aInstallDate : "1238441400314";
  var statusText = aStatusText ? aStatusText : "Install Pending";
  var isCompleteUpdate =
    typeof(aIsCompleteUpdate) == "string" ? aIsCompleteUpdate : "true";
  var channel = aChannel ? aChannel : "bogus_channel";
  var foregroundDownload =
    typeof(aForegroundDownload) == "string" ? aForegroundDownload : "true";
  return getUpdateString(aName, aType, aVersion, aPlatformVersion,
                         aExtensionVersion, aBuildID, aLicenseURL,
                         aDetailsURL) + " " +
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
































function getUpdateString(aName, aType, aVersion, aPlatformVersion,
                         aExtensionVersion, aBuildID, aLicenseURL, aDetailsURL) {
  var name = aName ? aName : "XPCShell App Update Test";
  var type = aType ? aType : "major";
  var version = aVersion ? aVersion : "4.0";
  var platformVersion = aPlatformVersion ? aPlatformVersion : "4.0";
  var extensionVersion = aExtensionVersion ? aExtensionVersion : "4.0";
  var buildID = aBuildID ? aBuildID : "20080811053724";
  var licenseURL = aLicenseURL ? aLicenseURL : "http://dummylicense/";
  var detailsURL = aDetailsURL ? aDetailsURL : "http://dummydetails/";
  return "  <update name=\"" + name + "\" " +
                   "type=\"" + type + "\" " +
                   "version=\"" + version + "\" " +
                   "platformVersion=\"" + platformVersion + "\" " +
                   "extensionVersion=\"" + extensionVersion + "\" " +
                   "buildID=\"" + buildID + "\" " +
                   "licenseURL=\"" + licenseURL + "\" " +
                   "detailsURL=\"" + detailsURL + "\"";
}






















function getPatchString(aType, aURL, aHashFunction, aHashValue, aSize) {
  var type = aType ? aType : "complete";
  var url = aURL ? aURL : URL_HOST + DIR_DATA + "/empty.mar";
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
  var file = getCurrentProcessDir();
  file.append("updates");
  file.append("0");
  file.append("update.status");
  aStatus += "\n";
  writeFile(file, aStatus);
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


function pathHandler(metadata, response) {
  response.setHeader("Content-Type", "text/xml", false);
  response.setStatusLine(metadata.httpVersion, gResponseStatusCode, "OK");
  response.bodyOutputStream.write(gResponseBody, gResponseBody.length);
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
















function overrideXHR(callback) {
  gXHRCallback = callback;
  gXHR = new xhr();
  var registrar = Components.manager.QueryInterface(AUS_Ci.nsIComponentRegistrar);
  registrar.registerFactory(gXHR.classID, gXHR.classDescription,
                            gXHR.contractID, gXHR);
}





function xhr() {
}
xhr.prototype = {
  overrideMimeType: function(mimetype) { },
  setRequestHeader: function(header, value) { },
  status: null,
  channel: { set notificationCallbacks(val) { } },
  _url: null,
  _method: null,
  open: function (method, url) {
    gXHR.channel.originalURI = AUS_Cc["@mozilla.org/network/io-service;1"].
                               getService(AUS_Ci.nsIIOService).
                               newURI(url, null, null);
    gXHR._method = method; gXHR._url = url;
  },
  responseXML: null,
  responseText: null,
  send: function(body) {
    do_timeout(0, "gXHRCallback()"); 
  },
  _onprogress: null,
  set onprogress(val) { gXHR._onprogress = val; },
  get onprogress() { return gXHR._onprogress; },
  _onerror: null,
  set onerror(val) { gXHR._onerror = val; },
  get onerror() { return gXHR._onerror; },
  _onload: null,
  set onload(val) { gXHR._onload = val; },
  get onload() { return gXHR._onload; },
  flags: AUS_Ci.nsIClassInfo.SINGLETON,
  implementationLanguage: AUS_Ci.nsIProgrammingLanguage.JAVASCRIPT,
  getHelperForLanguage: function(language) null,
  getInterfaces: function(count) {
    var interfaces = [AUS_Ci.nsIXMLHttpRequest, AUS_Ci.nsIJSXMLHttpRequest,
                      AUS_Ci.nsIXMLHttpRequestEventTarget];
    count.value = interfaces.length;
    return interfaces;
  },
  classDescription: "XMLHttpRequest",
  contractID: "@mozilla.org/xmlextras/xmlhttprequest;1",
  classID: Components.ID("{c9b37f43-4278-4304-a5e0-600991ab08cb}"),
  createInstance: function (outer, aIID) {
    if (outer == null)
      return gXHR.QueryInterface(aIID);
    throw AUS_Cr.NS_ERROR_NO_AGGREGATION;
  },
  QueryInterface: function(aIID) {
    if (aIID.equals(AUS_Ci.nsIXMLHttpRequest) ||
        aIID.equals(AUS_Ci.nsIJSXMLHttpRequest) ||
        aIID.equals(AUS_Ci.nsIXMLHttpRequestEventTarget) ||
        aIID.equals(AUS_Ci.nsIClassInfo) ||
        aIID.equals(AUS_Ci.nsISupports))
      return gXHR;
    throw AUS_Cr.NS_ERROR_NO_INTERFACE;
  }
};


const updateCheckListener = {
  onProgress: function(request, position, totalSize) {
  },

  onCheckComplete: function(request, updates, updateCount) {
    gRequestURL = request.channel.originalURI.spec;
    gUpdateCount = updateCount;
    gUpdates = updates;
    dump("onError: url = " + gRequestURL + ", " +
         "request.status = " + request.status + ", " +
         "update.statusText = " + request.statusText + ", " +
         "updateCount = " + updateCount + "\n");
    
    do_timeout(0, "gCheckFunc()");
  },

  onError: function(request, update) {
    gRequestURL = request.channel.originalURI.spec;
    gStatusCode = request.status;
    gStatusText = update.statusText;
    dump("onError: url = " + gRequestURL + ", " +
         "request.status = " + gStatusCode + ", " +
         "update.statusText = " + gStatusText + "\n");
    
    do_timeout(0, "gCheckFunc()");
  },

  QueryInterface: function(aIID) {
    if (!aIID.equals(AUS_Ci.nsIUpdateCheckListener) &&
        !aIID.equals(AUS_Ci.nsISupports))
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    return this;
  }
};






function removeUpdateDirsAndFiles() {
  var appDir = getCurrentProcessDir();
  var file = appDir.clone();
  file.append("active-update.xml");
  try {
    if (file.exists())
      file.remove(false);
  }
  catch (e) {
    dump("Unable to remove file\npath: " + file.path +
         "\nException: " + e + "\n");
  }

  file = appDir.clone();
  file.append("updates.xml");
  try {
    if (file.exists())
      file.remove(false);
  }
  catch (e) {
    dump("Unable to remove file\npath: " + file.path +
         "\nException: " + e + "\n");
  }

  file = appDir.clone();
  file.append("updates");
  file.append("last-update.log");
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
    removeDirRecursive(updatesDir);
  }
  catch (e) {
    dump("Unable to remove directory\npath: " + updatesDir.path +
         "\nException: " + e + "\n");
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







function start_httpserver(aRelativeDirName) {
  var dir = do_get_file(aRelativeDirName);
  if (!dir.exists())
    do_throw("The directory used by nsHttpServer does not exist! path: " +
             dir.path + "\n");

  if (!dir.isDirectory())
    do_throw("A file instead of a directory was specified for nsHttpServer " +
             "registerDirectory! path: " + dir.path + "\n");

  do_load_httpd_js();
  gTestserver = new nsHttpServer();
  gTestserver.registerDirectory("/data/", dir);
  gTestserver.start(4444);
}


function stop_httpserver(callback) {
  do_check_true(!!callback);
  gTestserver.stop(callback);
}












function createAppInfo(id, name, version, platformVersion) {
  const XULAPPINFO_CONTRACTID = "@mozilla.org/xre/app-info;1";
  const XULAPPINFO_CID = Components.ID("{c763b610-9d49-455a-bbd2-ede71682a1ac}");
  var XULAppInfo = {
    vendor: "Mozilla",
    name: name,
    ID: id,
    version: version,
    appBuildID: "2007010101",
    platformVersion: platformVersion,
    platformBuildID: "2007010101",
    inSafeMode: false,
    logConsoleErrors: true,
    OS: "XPCShell",
    XPCOMABI: "noarch-spidermonkey",

    QueryInterface: function QueryInterface(iid) {
      if (iid.equals(AUS_Ci.nsIXULAppInfo) ||
          iid.equals(AUS_Ci.nsIXULRuntime) ||
          iid.equals(AUS_Ci.nsISupports))
        return this;
      throw AUS_Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  
  var XULAppInfoFactory = {
    createInstance: function (outer, iid) {
      if (outer == null)
        return XULAppInfo.QueryInterface(iid);
      throw AUS_Cr.NS_ERROR_NO_AGGREGATION;
    }
  };

  var registrar = Components.manager.QueryInterface(AUS_Ci.nsIComponentRegistrar);
  registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                            XULAPPINFO_CONTRACTID, XULAppInfoFactory);
}







function getGREDir() {
  return gDirSvc.get(NS_GRE_DIR, AUS_Ci.nsIFile);
}






function getCurrentProcessDir() {
  return gDirSvc.get(NS_XPCOM_CURRENT_PROCESS_DIR, AUS_Ci.nsIFile);
}


var gProfD = do_get_cwd();
gProfD.append("profile");
if (gProfD.exists())
  gProfD.remove(true);
gProfD.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, PERMS_DIRECTORY);

var gDirProvider = {
  getFile: function(prop, persistent) {
    switch (prop) {
      case NS_APP_USER_PROFILE_50_DIR:
      case NS_APP_PROFILE_DIR_STARTUP:
        persistent.value = true;
        return gProfD.clone();
      case XRE_UPDATE_ROOT_DIR:
        persistent.value = true;
        return getCurrentProcessDir();
    }
    return null;
  },
  QueryInterface: function(iid) {
    if (iid.equals(AUS_Ci.nsIDirectoryServiceProvider) ||
        iid.equals(AUS_Ci.nsISupports))
      return this;
    throw AUS_Cr.NS_ERROR_NO_INTERFACE;
  }
};
gDirSvc.QueryInterface(AUS_Ci.nsIDirectoryService).registerProvider(gDirProvider);
