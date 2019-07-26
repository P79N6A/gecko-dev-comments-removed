function run_test()
{
  do_get_profile();

  
  asyncOpenCacheEntry("http://b/", "disk", Ci.nsICacheStorage.OPEN_READONLY, null,
    new OpenCallback(NOTFOUND, null, null, function(entry) {
      
      asyncOpenCacheEntry("http://b/", "disk", Ci.nsICacheStorage.OPEN_READONLY, null,
        new OpenCallback(NOTFOUND, null, null, function(entry) {
          
          asyncOpenCacheEntry("http://b/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
            new OpenCallback(NEW, "b1m", "b1d", function(entry) {
              
              asyncOpenCacheEntry("http://b/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
                new OpenCallback(NORMAL, "b1m", "b1d", function(entry) {
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
