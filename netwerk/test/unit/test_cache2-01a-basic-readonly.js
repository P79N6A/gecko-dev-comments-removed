function run_test()
{
  do_get_profile();

  
  asyncOpenCacheEntry("http://ro/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
    new OpenCallback(NEW, "a1m", "a1d", function(entry) {
      
      asyncOpenCacheEntry("http://ro/", "disk", Ci.nsICacheStorage.OPEN_READONLY, null,
        new OpenCallback(NORMAL, "a1m", "a1d", function(entry) {
          
          asyncOpenCacheEntry("http://ro/", "disk", Ci.nsICacheStorage.OPEN_TRUNCATE, null,
            new OpenCallback(NEW, "a2m", "a2d", function(entry) {
              
              asyncOpenCacheEntry("http://ro/", "disk", Ci.nsICacheStorage.OPEN_READONLY, null,
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
