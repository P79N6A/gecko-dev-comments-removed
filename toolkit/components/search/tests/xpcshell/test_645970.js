








function run_test() {
  updateAppInfo();

  do_load_manifest("data/chrome.manifest");

  configureToLoadJarEngines();

  
  
  let engine = Services.search.getEngineByName("bug645970");
  do_check_neq(engine, null);
  Services.obs.notifyObservers(null, "quit-application", null);
}
