






































const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

function LOG(aMsg) {
  aMsg = ("*** PLACES TESTS: " + aMsg);
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).
                                      logStringMessage(aMsg);
  print(aMsg);
}


var dirSvc = Cc["@mozilla.org/file/directory_service;1"].getService(Ci.nsIProperties);
var profileDir = null;
try {
  profileDir = dirSvc.get(NS_APP_USER_PROFILE_50_DIR, Ci.nsIFile);
} catch (e) {}
if (!profileDir) {
  
  
  var provider = {
    getFile: function(prop, persistent) {
      persistent.value = true;
      if (prop == NS_APP_USER_PROFILE_50_DIR) {
        return dirSvc.get("CurProcD", Ci.nsIFile);
      }
      throw Cr.NS_ERROR_FAILURE;
    },
    QueryInterface: function(iid) {
      if (iid.equals(Ci.nsIDirectoryServiceProvider) ||
          iid.equals(Ci.nsISupports)) {
        return this;
      }
      throw Cr.NS_ERROR_NO_INTERFACE;
    }
  };
  dirSvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(provider);
}

var XULAppInfo = {
  vendor: "Mozilla",
  name: "PlacesTest",
  ID: "{230de50e-4cd1-11dc-8314-0800200c9a66}",
  version: "1",
  appBuildID: "2007010101",
  platformVersion: "",
  platformBuildID: "2007010101",
  inSafeMode: false,
  logConsoleErrors: true,
  OS: "XPCShell",
  XPCOMABI: "noarch-spidermonkey",

  QueryInterface: function QueryInterface(iid) {
    if (iid.equals(Ci.nsIXULAppInfo) ||
        iid.equals(Ci.nsIXULRuntime) ||
        iid.equals(Ci.nsISupports))
      return this;
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};

var XULAppInfoFactory = {
  createInstance: function (outer, iid) {
    if (outer != null)
      throw Cr.NS_ERROR_NO_AGGREGATION;
    return XULAppInfo.QueryInterface(iid);
  }
};

var registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
registrar.registerFactory(Components.ID("{fbfae60b-64a4-44ef-a911-08ceb70b9f31}"),
                          "XULAppInfo", "@mozilla.org/xre/app-info;1",
                          XULAppInfoFactory);

var updateSvc = Cc["@mozilla.org/updates/update-service;1"].
                getService(Ci.nsISupports);

var iosvc = Cc["@mozilla.org/network/io-service;1"].getService(Ci.nsIIOService);

function uri(spec) {
  return iosvc.newURI(spec, null, null);
}

function cleanUp() {
  try {
    
    var file = dirSvc.get('ProfD', Ci.nsIFile);
    file.append("places.sqlite");
    if (file.exists())
      file.remove(false);

    
    file = dirSvc.get('ProfD', Ci.nsIFile);
    file.append("bookmarks.exported.html");
    if (file.exists())
      file.remove(false);
  } catch(ex) { dump("Exception: " + ex); }
}
cleanUp();
