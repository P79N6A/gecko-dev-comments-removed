




const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;
const Cu = Components.utils;

Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/LoadContextInfo.jsm");


let commonFile = do_get_file("../../../../../toolkit/components/places/tests/head_common.js", false);
if (commonFile) {
  let uri = Services.io.newFileURI(commonFile);
  Services.scriptloader.loadSubScript(uri.spec, this);
}



XPCOMUtils.defineLazyGetter(this, "PlacesUIUtils", function() {
  Cu.import("resource:///modules/PlacesUIUtils.jsm");
  return PlacesUIUtils;
});

const ORGANIZER_FOLDER_ANNO = "PlacesOrganizer/OrganizerFolder";
const ORGANIZER_QUERY_ANNO = "PlacesOrganizer/OrganizerQuery";


let XULAppInfo = {
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
};

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


const SMART_BOOKMARKS_VERSION = 7;
const SMART_BOOKMARKS_ON_TOOLBAR = 1;
const SMART_BOOKMARKS_ON_MENU =  3; 


const DEFAULT_BOOKMARKS_ON_TOOLBAR = 1;
const DEFAULT_BOOKMARKS_ON_MENU = 1;

const SMART_BOOKMARKS_ANNO = "Places/SmartBookmark";

function checkItemHasAnnotation(guid, name) {
  return PlacesUtils.promiseItemId(guid).then(id => {
    let hasAnnotation = PlacesUtils.annotations.itemHasAnnotation(id, name);
    Assert.ok(hasAnnotation, `Expected annotation ${name}`);
  });
}

let createCorruptDB = Task.async(function* () {
  let dbPath = OS.Path.join(OS.Constants.Path.profileDir, "places.sqlite");
  yield OS.File.remove(dbPath);

  
  let dir = yield OS.File.getCurrentDirectory();
  let src = OS.Path.join(dir, "corruptDB.sqlite");
  yield OS.File.copy(src, dbPath);

  
  Assert.ok((yield OS.File.exists(dbPath)), "should have a DB now");
});








function rebuildSmartBookmarks() {
  let consoleListener = {
    observe(aMsg) {
      do_throw("Got console message: " + aMsg.message);
    },
    QueryInterface: XPCOMUtils.generateQI([ Ci.nsIConsoleListener ]),
  };
  Services.console.reset();
  Services.console.registerListener(consoleListener);
  do_register_cleanup(() => {
    try {
      Services.console.unregisterListener(consoleListener);
    } catch (ex) {  }
  });
  Cc["@mozilla.org/browser/browserglue;1"]
    .getService(Ci.nsIObserver)
    .observe(null, "browser-glue-test", "smart-bookmarks-init");
  return promiseTopicObserved("test-smart-bookmarks-done").then(() => {
    Services.console.unregisterListener(consoleListener);
  });
}
