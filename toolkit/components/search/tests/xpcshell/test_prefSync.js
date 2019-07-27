







"use strict";

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

function run_test() {
  removeMetadata();
  updateAppInfo();
  useHttpServer();

  run_next_test();
}

add_task(function* test_prefSync() {
  let [engine1, engine2] = yield addTestEngines([
    { name: "Test search engine", xmlFileName: "engine.xml" },
    { name: "A second test engine", xmlFileName: "engine2.xml" },
  ]);

  let search = Services.search;

  
  search.defaultEngine = engine1;
  do_check_eq(search.defaultEngine, engine1);
  search.currentEngine = engine1;
  do_check_eq(search.currentEngine, engine1);

  setLocalizedPref("defaultenginename", engine2.name);
  
  do_check_eq(search.defaultEngine, engine2);
  
  do_check_eq(search.currentEngine, engine1);
  
  setLocalizedPref("selectedEngine", engine2.name);
  
  do_check_eq(search.defaultEngine, engine2);
  
  do_check_eq(search.currentEngine, engine2);

  
  
  
  let defaultBranch = Services.prefs.getDefaultBranch("");
  let prefName = PREF_BRANCH + "defaultenginename";
  let prefVal = "data:text/plain," + prefName + "=" + engine1.name;
  defaultBranch.setCharPref(prefName, prefVal, true);
  search.currentEngine = engine1;
  
  do_check_eq(search.currentEngine, engine1);
  do_check_false(Services.prefs.prefHasUserValue("browser.search.selectedEngine"));

  
  
  do_check_true(Services.prefs.prefHasUserValue("browser.search.defaultenginename"));
  search.defaultEngine = engine1;
  do_check_eq(search.defaultEngine, engine1);
  do_check_false(Services.prefs.prefHasUserValue("browser.search.defaultenginename"));
});
