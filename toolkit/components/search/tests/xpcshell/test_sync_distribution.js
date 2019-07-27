


function run_test() {
  removeMetadata();
  removeCacheFile();

  do_load_manifest("data/chrome.manifest");

  configureToLoadJarEngines();
  installDistributionEngine();

  do_check_false(Services.search.isInitialized);

  
  let engines = Services.search.getEngines();
  do_check_eq(engines.length, 1);

  do_check_true(Services.search.isInitialized);

  let engine = Services.search.getEngineByName("bug645970");
  do_check_neq(engine, null);

  
  do_check_eq(engine.description, "override");
}
