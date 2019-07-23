






































version(170);

const NS_APP_USER_PROFILE_50_DIR = "ProfD";
const NS_APP_PROFILE_DIR_STARTUP = "ProfDS";
var Ci = Components.interfaces;
var Cc = Components.classes;
var Cr = Components.results;

function LOG(aMsg) {
  aMsg = ("*** PLACES TESTS: " + aMsg);
  Cc["@mozilla.org/consoleservice;1"].getService(Ci.nsIConsoleService).
                                      logStringMessage(aMsg);
  print(aMsg);
}

var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);
var gTestRoot = dirSvc.get("CurProcD", Ci.nsILocalFile);
gTestRoot = gTestRoot.parent.parent;
gTestRoot.append("_tests");
gTestRoot.append("xpcshell-simple");
gTestRoot.append("test_browser_places");
gTestRoot.normalize();


var gProfD = gTestRoot.clone();
gProfD.append("profile");
if (gProfD.exists())
  gProfD.remove(true);
gProfD.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);

var dirProvider = {
  getFile: function(prop, persistent) {
    persistent.value = true;
    if (prop == NS_APP_USER_PROFILE_50_DIR ||
        prop == NS_APP_PROFILE_DIR_STARTUP)
      return gProfD.clone();
    return null;
  },
  QueryInterface: function(iid) {
    if (iid.equals(Ci.nsIDirectoryServiceProvider) ||
        iid.equals(Ci.nsISupports)) {
      return this;
    }
    throw Cr.NS_ERROR_NO_INTERFACE;
  }
};
dirSvc.QueryInterface(Ci.nsIDirectoryService).registerProvider(dirProvider);

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



Cc["@mozilla.org/browser/livemark-service;2"].getService(Ci.nsILivemarkService);
Cc["@mozilla.org/feed-processor;1"].createInstance(Ci.nsIFeedProcessor);

function uri(spec) {
  return iosvc.newURI(spec, null, null);
}




function remove_all_bookmarks() {
  var bs = Cc["@mozilla.org/browser/nav-bookmarks-service;1"].
           getService(Ci.nsINavBookmarksService);
  
  bs.removeFolderChildren(bs.bookmarksMenuFolder);
  bs.removeFolderChildren(bs.toolbarFolder);
  bs.removeFolderChildren(bs.unfiledBookmarksFolder);
  
  check_no_bookmarks()
}




function check_no_bookmarks() {
  var hs = Cc["@mozilla.org/browser/nav-history-service;1"].
           getService(Ci.nsINavHistoryService);
  var query = hs.getNewQuery();
  query.setFolders([bs.toolbarFolder, bs.bookmarksMenuFolder, bs.unfiledBookmarksFolder], 3);
  var options = hs.getNewQueryOptions();
  options.queryType = Ci.nsINavHistoryQueryOptions.QUERY_TYPE_BOOKMARKS;
  var result = hs.executeQuery(query, options);
  var root = result.root;
  root.containerOpen = true;
  do_check_eq(root.childCount, 0);
  root.containerOpen = false;
}
