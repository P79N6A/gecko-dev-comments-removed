function run_test()
{
  do_get_profile();

  
  asyncOpenCacheEntry("http://d/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
    new OpenCallback(NEW|THROWAVAIL, null, null, function(entry) {
      
      asyncOpenCacheEntry("http://d/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
        new OpenCallback(NEW|THROWAVAIL, null, null, function(entry) {
          
          asyncOpenCacheEntry("http://d/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
            new OpenCallback(NEW, "d1m", "d1d", function(entry) {
              
              asyncOpenCacheEntry("http://d/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
                new OpenCallback(NORMAL, "d1m", "d1d", function(entry) {
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
