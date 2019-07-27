






"use strict";

const SEARCH_APP_DIR = 1;

function run_test() {
  removeMetadata();
  removeCacheFile();
  do_load_manifest("data/chrome.manifest");

  configureToLoadJarEngines();

  updateAppInfo();

  run_next_test();
}

add_test(function test_identifier() {
  let engineFile = gProfD.clone();
  engineFile.append("searchplugins");
  engineFile.append("test-search-engine.xml");
  engineFile.parent.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

  
  let engineTemplateFile = do_get_file("data/engine.xml");
  engineTemplateFile.copyTo(engineFile.parent, "test-search-engine.xml");

  let search = Services.search.init(function initComplete(aResult) {
    do_print("init'd search service");
    do_check_true(Components.isSuccessCode(aResult));

    let profileEngine = Services.search.getEngineByName("Test search engine");
    let jarEngine = Services.search.getEngineByName("bug645970");

    do_check_true(profileEngine instanceof Ci.nsISearchEngine);
    do_check_true(jarEngine instanceof Ci.nsISearchEngine);

    
    
    do_check_eq(profileEngine.identifier, null);

    
    
    do_check_eq(jarEngine.identifier, "bug645970");

    removeMetadata();
    removeCacheFile();
    run_next_test();
  });
});
