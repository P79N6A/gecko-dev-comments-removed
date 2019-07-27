


function run_test() {
  do_test_pending();

  removeMetadata();
  removeCacheFile();

  do_load_manifest("data/chrome.manifest");

  configureToLoadJarEngines();
  installAddonEngine("engine-override");

  do_check_false(Services.search.isInitialized);

  Services.search.init(function search_initialized(aStatus) {
    do_check_true(Components.isSuccessCode(aStatus));
    do_check_true(Services.search.isInitialized);

    
    let engines = Services.search.getEngines();
    do_check_eq(engines.length, 1);

    
    let engine = Services.search.getEngineByName("bug645970");
    do_check_neq(engine, null);

    do_check_eq(engine.description, "bug645970");

    do_test_finished();
  });
}
