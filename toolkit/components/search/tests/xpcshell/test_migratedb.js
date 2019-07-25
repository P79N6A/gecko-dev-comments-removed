










function run_test()
{
  removeMetadata();

  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "2");

  let search_sqlite = do_get_file("data/search.sqlite");
  search_sqlite.copyTo(gProfD, "search.sqlite");

  let search = Services.search;

  do_test_pending();
  afterCommit(
    function()
    {
      
      let metadata = gProfD.clone();
      metadata.append("search-metadata.json");
      do_check_true(metadata.exists());

      removeMetadata();
      do_test_finished();
    }
  );

  search.getEngines();
}
