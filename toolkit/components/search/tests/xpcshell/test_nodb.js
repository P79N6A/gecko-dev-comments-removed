













function run_test()
{
  removeMetadata();
  updateAppInfo();

  let search = Services.search;

  do_test_pending();
  search.init(function ss_initialized(rv) {
    do_check_true(Components.isSuccessCode(rv));
    do_timeout(500, function() {
      
      
      
      let metadata = gProfD.clone();
      metadata.append("search-metadata.json");
      do_check_true(!metadata.exists());
      removeMetadata();

      do_test_finished();
    });
  });
}
