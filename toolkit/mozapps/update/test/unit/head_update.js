







































const AUS_Cc = Components.classes;
const AUS_Ci = Components.interfaces;
const AUS_Cr = Components.results;

const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const NS_APP_PROFILE_DIR_STARTUP = "ProfDS";
const NS_GRE_DIR                 = "GreD";

const MODE_RDONLY   = 0x01;
const MODE_WRONLY   = 0x02;
const MODE_CREATE   = 0x08;
const MODE_APPEND   = 0x10;
const MODE_TRUNCATE = 0x20;

const PERMS_FILE      = 0644;
const PERMS_DIRECTORY = 0755;

var gAUS           = null;
var gUpdateChecker = null;
var gPrefs         = null;
var gTestserver    = null;
var gXHR           = null;
var gXHRCallback   = null;


function startAUS() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1.0", "2.0");
  gPrefs = AUS_Cc["@mozilla.org/preferences;1"]
             .getService(AUS_Ci.nsIPrefBranch);

  
  gPrefs.setBoolPref("app.update.log.all", true);
  
  gPrefs.setBoolPref("app.update.enabled", false);
  gPrefs.setBoolPref("extensions.blocklist.enabled", false);
  gPrefs.setBoolPref("extensions.update.enabled", false);
  gPrefs.setBoolPref("browser.search.update", false);
  gPrefs.setBoolPref("browser.microsummary.updateGenerators", false);

  gAUS = AUS_Cc["@mozilla.org/updates/update-service;1"]
           .getService(AUS_Ci.nsIApplicationUpdateService);
  gUpdateChecker = AUS_Cc["@mozilla.org/updates/update-checker;1"]
                     .createInstance(AUS_Ci.nsIUpdateChecker);
  
  





}










function writeFile(aFile, aText) {
  var fos = AUS_Cc["@mozilla.org/network/safe-file-output-stream;1"]
              .createInstance(AUS_Ci.nsIFileOutputStream);
  if (!aFile.exists())
    aFile.create(AUS_Ci.nsILocalFile.NORMAL_FILE_TYPE, PERMS_FILE);
  var modeFlags = MODE_WRONLY | MODE_CREATE | MODE_TRUNCATE;
  fos.init(aFile, modeFlags, PERMS_FILE, 0);
  fos.write(aText, aText.length);
  closeSafeOutputStream(fos);
}






function closeSafeOutputStream(aFOS) {
  if (aFOS instanceof AUS_Ci.nsISafeOutputStream) {
    try {
      aFOS.finish();
    }
    catch (e) {
      aFOS.close();
    }
  }
  else
    aFOS.close();
}









function toggleOffline(aOffline) {
  const ioService = AUS_Cc["@mozilla.org/network/io-service;1"]
                      .getService(AUS_Ci.nsIIOService);

  try {
    ioService.manageOfflineStatus = !aOffline;
  }
  catch (e) {
  }
  if (ioService.offline != aOffline)
    ioService.offline = aOffline;
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
  open: function (method, url) { gXHR._method = method; gXHR._url = url; },
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
    if (outer != null)
      throw AUS_Cr.NS_ERROR_NO_AGGREGATION;
    return gXHR.QueryInterface(aIID);
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






function remove_dirs_and_files () {
  var dir = gDirSvc.get(NS_GRE_DIR, AUS_Ci.nsIFile);

  var file = dir.clone();
  file.append("active-update.xml");
  try {
    if (file.exists())
      file.remove(false);
  }
  catch (e) {
    dump("Unable to remove file\npath: " + file.path +
         "\nException: " + e + "\n");
  }

  file = dir.clone();
  file.append("updates.xml");
  try {
    if (file.exists())
      file.remove(false);
  }
  catch (e) {
    dump("Unable to remove file\npath: " + file.path +
         "\nException: " + e + "\n");
  }

  file = dir.clone();
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

  var updatesSubDir = dir.clone();
  updatesSubDir.append("updates");
  updatesSubDir.append("0");
  if (updatesSubDir.exists()) {
    file = updatesSubDir.clone();
    file.append("update.mar");
    try {
      if (file.exists())
        file.remove(false);
    }
    catch (e) {
      dump("Unable to remove file\npath: " + file.path +
           "\nException: " + e + "\n");
    }

    file = updatesSubDir.clone();
    file.append("update.status");
    try {
      if (file.exists())
        file.remove(false);
    }
    catch (e) {
      dump("Unable to remove file\npath: " + file.path +
           "\nException: " + e + "\n");
    }

    file = updatesSubDir.clone();
    file.append("update.version");
    try {
      if (file.exists())
        file.remove(false);
    }
    catch (e) {
      dump("Unable to remove file\npath: " + file.path +
           "\nException: " + e + "\n");
    }

    try {
      updatesSubDir.remove(true);
    }
    catch (e) {
      dump("Unable to remove directory\npath: " + updatesSubDir.path +
           "\nException: " + e + "\n");
    }
  }

  
  dir.append("updates");
  try {
    if (dir.exists())
      dir.remove(true);
  }
  catch (e) {
    dump("Unable to remove directory\npath: " + dir.path +
         "\nException: " + e + "\n");
  }
}







function start_httpserver(aRelativeDirName) {
  do_load_httpd_js();
  gTestserver = new nsHttpServer();
  gTestserver.registerDirectory("/data/", do_get_file(aRelativeDirName));
  gTestserver.start(4444);
}


function stop_httpserver(callback) {
  do_check_true(!!callback);
  gTestserver.stop(callback);
}












function createAppInfo(id, name, version, platformVersion)
{
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
      if (outer != null)
        throw AUS_Cr.NS_ERROR_NO_AGGREGATION;
      return XULAppInfo.QueryInterface(iid);
    }
  };

  var registrar = Components.manager.QueryInterface(AUS_Ci.nsIComponentRegistrar);
  registrar.registerFactory(XULAPPINFO_CID, "XULAppInfo",
                            XULAPPINFO_CONTRACTID, XULAppInfoFactory);
}


var gDirSvc = AUS_Cc["@mozilla.org/file/directory_service;1"].
             getService(AUS_Ci.nsIProperties);

var gTestRoot = __LOCATION__.parent.parent;
gTestRoot.normalize();


var gProfD = gTestRoot.clone();
gProfD.append("profile");
if (gProfD.exists())
  gProfD.remove(true);
gProfD.create(AUS_Ci.nsIFile.DIRECTORY_TYPE, 0755);

var dirProvider = {
  getFile: function(prop, persistent) {
    persistent.value = true;
    if (prop == NS_APP_USER_PROFILE_50_DIR ||
        prop == NS_APP_PROFILE_DIR_STARTUP)
      return gProfD.clone();
    return null;
  },
  QueryInterface: function(iid) {
    if (iid.equals(AUS_Ci.nsIDirectoryServiceProvider) ||
        iid.equals(AUS_Ci.nsISupports)) {
      return this;
    }
    throw AUS_Cr.NS_ERROR_NO_INTERFACE;
  }
};
gDirSvc.QueryInterface(AUS_Ci.nsIDirectoryService).registerProvider(dirProvider);

remove_dirs_and_files();
