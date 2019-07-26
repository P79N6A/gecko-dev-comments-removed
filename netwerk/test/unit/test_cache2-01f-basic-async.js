function run_test()
{
  do_get_profile();

  
  asyncOpenCacheEntry("http://a/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY | Ci.nsICacheStorage.FORCE_ASYNC_CALLBACK, null,
    new OpenCallback(NEW, "a1m", "a1d", function(entry) {
      
      asyncOpenCacheEntry("http://a/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY | Ci.nsICacheStorage.FORCE_ASYNC_CALLBACK, null,
        new OpenCallback(NORMAL, "a1m", "a1d", function(entry) {
          
          asyncOpenCacheEntry("http://a/", "disk", Ci.nsICacheStorage.OPEN_TRUNCATE | Ci.nsICacheStorage.FORCE_ASYNC_CALLBACK, null,
            new OpenCallback(NEW, "a2m", "a2d", function(entry) {
              
              asyncOpenCacheEntry("http://a/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY | Ci.nsICacheStorage.FORCE_ASYNC_CALLBACK, null,
                new OpenCallback(NORMAL, "a2m", "a2d", function(entry) {
                  finish_cache2_test();
                })
              );
            })
          );
        })
      );
    })
  );

  do_test_pending();
}
