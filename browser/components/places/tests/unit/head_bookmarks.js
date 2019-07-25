






































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");


let (commonFile = do_get_file("../../../../../toolkit/components/places/tests/head_common.js", false)) {
  let uri = Services.io.newFileURI(commonFile);
  Services.scriptloader.loadSubScript(uri.spec, this);
}




XPCOMUtils.defineLazyGetter(this, "PlacesUIUtils", function() {
  Cu.import("resource:///modules/PlacesUIUtils.jsm");
  return PlacesUIUtils;
});


const ORGANIZER_FOLDER_ANNO = "PlacesOrganizer/OrganizerFolder";
const ORGANIZER_QUERY_ANNO = "PlacesOrganizer/OrganizerQuery";



let (XULAppInfo = {
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

  QueryInterface: XPCOMUtils.generateQI([
    Ci.nsIXULAppInfo,
    Ci.nsIXULRuntime,
  ])
}) {
  let XULAppInfoFactory = {
    createInstance: function (outer, iid) {
      if (outer != null)
        throw Cr.NS_ERROR_NO_AGGREGATION;
      return XULAppInfo.QueryInterface(iid);
    }
  };
  let registrar = Components.manager.QueryInterface(Ci.nsIComponentRegistrar);
  registrar.registerFactory(Components.ID("{fbfae60b-64a4-44ef-a911-08ceb70b9f31}"),
                            "XULAppInfo", "@mozilla.org/xre/app-info;1",
                            XULAppInfoFactory);
}


const FILENAME_BOOKMARKS_HTML = "bookmarks.html";
let (backup_date = new Date().toLocaleFormat("%Y-%m-%d")) {
  const FILENAME_BOOKMARKS_JSON = "bookmarks-" + backup_date + ".json";
}


const SMART_BOOKMARKS_VERSION = 2;
const SMART_BOOKMARKS_ON_TOOLBAR = 1;
const SMART_BOOKMARKS_ON_MENU = 3; 


const DEFAULT_BOOKMARKS_ON_TOOLBAR = 1;
const DEFAULT_BOOKMARKS_ON_MENU = 1;
