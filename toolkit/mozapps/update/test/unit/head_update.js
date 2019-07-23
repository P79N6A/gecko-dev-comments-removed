





































const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const NS_APP_PROFILE_DIR_STARTUP = "ProfDS";



const AUS_Cc = Components.classes;
const AUS_Ci = Components.interfaces;
const AUS_Cr = Components.results;

var gAUS           = null;
var gUpdateChecker = null;
var gPrefs         = null;
var gTestserver    = null;


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






function remove_dirs_and_files () {
  var fileLocator = AUS_Cc["@mozilla.org/file/directory_service;1"]
                      .getService(AUS_Ci.nsIProperties);
  var dir = fileLocator.get("XCurProcD", AUS_Ci.nsIFile);

  var file = dir.clone();
  file.append("active-update.xml");
  if (file.exists())
    file.remove(false);

  file = dir.clone();
  file.append("updates.xml");
  if (file.exists())
    file.remove(false);

  dir.append("updates");
  if (dir.exists())
    dir.remove(true);
}







function start_httpserver(aRelativeDirName) {
  do_import_script("netwerk/test/httpserver/httpd.js");
  gTestserver = new nsHttpServer();
  gTestserver.registerDirectory("/data/", do_get_file("toolkit/mozapps/update/test/unit/" + aRelativeDirName));
  gTestserver.start(4444);
}


function stop_httpserver() {
  gTestserver.stop();
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
var gTestRoot = gDirSvc.get("CurProcD", AUS_Ci.nsILocalFile);
gTestRoot = gTestRoot.parent.parent;
gTestRoot.append("_tests");
gTestRoot.append("xpcshell-simple");
gTestRoot.append("test_update");
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
