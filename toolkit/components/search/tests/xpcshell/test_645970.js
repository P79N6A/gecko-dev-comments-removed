








function run_test() {
  updateAppInfo();

  do_load_manifest("data/chrome.manifest");

  let url  = "chrome://testsearchplugin/locale/searchplugins/";
  Services.prefs.setCharPref("browser.search.jarURIs", url);

  Services.prefs.setBoolPref("browser.search.loadFromJars", true);

  
  
  let engine = Services.search.getEngineByName("bug645970");
  do_check_neq(engine, null);
  Services.obs.notifyObservers(null, "quit-application", null);
}

