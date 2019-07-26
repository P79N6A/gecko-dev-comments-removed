function run_test()
{
  do_get_profile();

  if (!newCacheBackEndUsed()) {
    do_check_true(true, "This test doesn't run when the old cache back end is used since the behavior is different");
    return;
  }

  
  asyncOpenCacheEntry("http://a/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
    new OpenCallback(NEW|DONTFILL, "a1m", "a1d", function(entry) {
      var bypassed = false;

      
      asyncOpenCacheEntry("http://a/", "disk", Ci.nsICacheStorage.OPEN_BYPASS_IF_BUSY, null,
        new OpenCallback(NOTFOUND, "", "", function(entry) {
          do_check_false(bypassed);
          bypassed = true;
        })
      );

      
      
      
      
      do_execute_soon(function() {
        do_check_true(bypassed);
        finish_cache2_test();
      });
    })
  );

  do_test_pending();
}
