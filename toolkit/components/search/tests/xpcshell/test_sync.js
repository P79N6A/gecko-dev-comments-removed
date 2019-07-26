


function run_test() {
  removeMetadata();
  removeCacheFile();

  do_load_manifest("data/chrome.manifest");

  let url  = "chrome://testsearchplugin/locale/searchplugins/";
  Services.prefs.setCharPref("browser.search.jarURIs", url);
  Services.prefs.setBoolPref("browser.search.loadFromJars", true);

  do_check_false(Services.search.isInitialized);

  
  let engines = Services.search.getEngines();
  do_check_true(engines.length > 1);

  do_check_true(Services.search.isInitialized);

  
  let engine = Services.search.getEngineByName("bug645970");
  do_check_neq(engine, null);

  Services.prefs.clearUserPref("browser.search.jarURIs");
  Services.prefs.clearUserPref("browser.search.loadFromJars");
}

