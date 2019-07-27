


function run_test() {
  do_test_pending();

  removeMetadata();
  removeCacheFile();

  do_load_manifest("data/chrome.manifest");

  configureToLoadJarEngines();

  do_check_false(Services.search.isInitialized);
  let fallback = false;

  Services.search.init(function search_initialized(aStatus) {
    do_check_true(fallback);
    do_check_true(Components.isSuccessCode(aStatus));
    do_check_true(Services.search.isInitialized);

    
    let engines = Services.search.getEngines();
    do_check_eq(engines.length, 1);

    
    let engine = Services.search.getEngineByName("bug645970");
    do_check_neq(engine, null);

    do_test_finished();
  });

  
  Services.obs.addObserver(function searchServiceObserver(aResult, aTopic, aVerb) {
    if (aVerb == "find-jar-engines") {
      Services.obs.removeObserver(searchServiceObserver, aTopic);
      fallback = true;

      do_check_false(Services.search.isInitialized);

      
      let engines = Services.search.getEngines();
      do_check_eq(engines.length, 1);

      
      let engine = Services.search.getEngineByName("bug645970");
      do_check_neq(engine, null);

      do_check_true(Services.search.isInitialized);
    }
  }, "browser-search-service", false);
}
