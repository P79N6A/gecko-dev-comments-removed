function run_test()
{
  if (!newCacheBackEndUsed()) {
    do_check_true(true, "This test doesn't run when the old cache back end is used since the behavior is different");
    return;
  }

  do_get_profile();

  
  
  
  asyncOpenCacheEntry("http://nv/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
    new OpenCallback(NEW|DOOMED, "v1m", "v1d", function(entry) {
      
      asyncOpenCacheEntry("http://nv/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
        new OpenCallback(NOTVALID|RECREATE, "v2m", "v2d", function(entry) {
          
          asyncOpenCacheEntry("http://nv/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
            new OpenCallback(NORMAL, "v2m", "v2d", function(entry) {
              finish_cache2_test();
            })
          );
        })
      );
    })
  );

  do_test_pending();
}
