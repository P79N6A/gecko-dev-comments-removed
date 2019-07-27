






function run_test() {
  do_test_pending();

  removeMetadata();
  removeCacheFile();

  do_check_false(Services.search.isInitialized);

  let engineFile = gProfD.clone();
  engineFile.append("searchplugins");
  engineFile.append("test-search-engine.xml");
  engineFile.parent.create(Ci.nsIFile.DIRECTORY_TYPE, FileUtils.PERMS_DIRECTORY);

  
  let engineTemplateFile = do_get_file("data/invalid-engine.xml");
  engineTemplateFile.copyTo(engineFile.parent, "test-search-engine.xml");

  Services.search.init(function search_initialized(aStatus) {
    
    
    do_check_true(Components.isSuccessCode(aStatus));
    do_check_true(Services.search.isInitialized);

    removeMetadata();
    removeCacheFile();
    do_test_finished();
  });
}
