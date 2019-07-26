







"use strict";

const { classes: Cc, interfaces: Ci, utils: Cu, results: Cr } = Components;

Cu.import("resource://testing-common/httpd.js");

let waitForEngines = {
  "Test search engine": 1,
  "A second test engine": 1
};

const PREF_BRANCH = "browser.search.";






function setLocalizedPref(aPrefName, aValue) {
  let nsIPLS = Ci.nsIPrefLocalizedString;
  let branch = Services.prefs.getBranch(PREF_BRANCH);
  try {
    var pls = Cc["@mozilla.org/pref-localizedstring;1"].
              createInstance(Ci.nsIPrefLocalizedString);
    pls.data = aValue;
    branch.setComplexValue(aPrefName, nsIPLS, pls);
  } catch (ex) {}
}

function search_observer(aSubject, aTopic, aData) {
  let engine = aSubject.QueryInterface(Ci.nsISearchEngine);
  do_print("Observer: " + aData + " for " + engine.name);

  if (aData != "engine-added") {
    return;
  }

  
  if (waitForEngines[engine.name]) {
    delete waitForEngines[engine.name];
  } else {
    
    return;
  }

  
  if (Object.keys(waitForEngines).length) {
    return;
  }

  let search = Services.search;

  let engine1Name = "Test search engine";
  let engine2Name = "A second test engine";
  let engine1 = search.getEngineByName(engine1Name);
  let engine2 = search.getEngineByName(engine2Name);

  
  search.defaultEngine = engine1;
  do_check_eq(search.defaultEngine, engine1);
  search.currentEngine = engine1;
  do_check_eq(search.currentEngine, engine1);

  setLocalizedPref("defaultenginename", engine2Name);
  
  do_check_eq(search.defaultEngine, engine2);
  
  do_check_eq(search.currentEngine, engine1);
  
  setLocalizedPref("selectedEngine", engine2Name);
  
  do_check_eq(search.defaultEngine, engine2);
  
  do_check_eq(search.currentEngine, engine2);

  
  
  
  let defaultBranch = Services.prefs.getDefaultBranch("");
  let prefName = PREF_BRANCH + "defaultenginename";
  let prefVal = "data:text/plain," + prefName + "=" + engine1Name;
  defaultBranch.setCharPref(prefName, prefVal, true);
  search.currentEngine = engine1;
  
  do_check_eq(search.currentEngine, engine1);
  do_check_false(Services.prefs.prefHasUserValue("browser.search.selectedEngine"));

  
  
  do_check_true(Services.prefs.prefHasUserValue("browser.search.defaultenginename"));
  search.defaultEngine = engine1;
  do_check_eq(search.defaultEngine, engine1);
  do_check_false(Services.prefs.prefHasUserValue("browser.search.defaultenginename"));

  do_test_finished();
}

function run_test() {
  removeMetadata();
  updateAppInfo();

  let httpServer = new HttpServer();
  httpServer.start(-1);
  httpServer.registerDirectory("/", do_get_cwd());
  let baseUrl = "http://localhost:" + httpServer.identity.primaryPort;

  do_register_cleanup(function cleanup() {
    httpServer.stop(function() {});
    Services.obs.removeObserver(search_observer, "browser-search-engine-modified");
  });

  do_test_pending();

  Services.obs.addObserver(search_observer, "browser-search-engine-modified", false);

  Services.search.addEngine(baseUrl + "/data/engine.xml",
                   Ci.nsISearchEngine.DATA_XML,
                   null, false);
  Services.search.addEngine(baseUrl + "/data/engine2.xml",
                   Ci.nsISearchEngine.DATA_XML,
                   null, false);
}
