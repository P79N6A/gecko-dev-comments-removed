



function continue_test(status, entry) {
  do_check_eq(status, Components.results.NS_OK);
  entry.deviceID;
  
  do_test_finished();
}

function run_test() {
  asyncOpenCacheEntry("key",
                      "client",
                      Components.interfaces.nsICache.STORE_ANYWHERE,
                      Components.interfaces.nsICache.ACCESS_WRITE,
                      continue_test);
  do_test_pending();
}
