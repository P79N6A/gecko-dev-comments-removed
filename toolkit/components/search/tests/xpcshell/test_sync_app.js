


function run_test() {
  removeMetadata();
  removeCacheFile();

  do_load_manifest("data/chrome.manifest");

  configureToLoadJarEngines(false);

  do_check_false(Services.search.isInitialized);

  
  let engine = Services.search.getEngineByName("TestEngineApp");
  do_check_neq(engine, null);

  do_check_true(Services.search.isInitialized);

  
  engine = Services.search.getEngineByName("bug645970");
  do_check_eq(engine, null);
}
