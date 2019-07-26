










function run_test()
{
  removeMetadata();

  updateAppInfo();

  let search_sqlite = do_get_file("data/search.sqlite");
  search_sqlite.copyTo(gProfD, "search.sqlite");

  let search = Services.search;

  do_test_pending();

  afterCommit(function commit_complete() {
    
    let metadata = gProfD.clone();
    metadata.append("search-metadata.json");
    do_check_true(metadata.exists());

    removeMetadata();
    do_test_finished();
  });

  search.init(function ss_initialized(rv) {
    do_check_true(Components.isSuccessCode(rv));
    search.getEngines();
  });
}
