



function continue_test(status, entry) {
  do_check_eq(status, Components.results.NS_OK);
  
  
  
  do_test_finished();
}

function run_test() {
  asyncOpenCacheEntry("http://some.key/",
                      "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
                      continue_test);
  do_test_pending();
}
