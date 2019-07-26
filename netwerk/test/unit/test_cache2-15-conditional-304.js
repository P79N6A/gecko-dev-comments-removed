function run_test()
{
  do_get_profile();

  
  asyncOpenCacheEntry("http://304/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
    new OpenCallback(NEW, "31m", "31d", function(entry) {
      
      asyncOpenCacheEntry("http://304/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
        new OpenCallback(REVAL, "31m", "31d", function(entry) {
          
          do_execute_soon(function() {
            entry.setValid(); 
          });
        })
      );

      var mc = new MultipleCallbacks(3, finish_cache2_test);

      asyncOpenCacheEntry("http://304/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
        new OpenCallback(NORMAL, "31m", "31d", function(entry) {
          mc.fired();
        })
      );
      asyncOpenCacheEntry("http://304/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
        new OpenCallback(NORMAL, "31m", "31d", function(entry) {
          mc.fired();
        })
      );
      asyncOpenCacheEntry("http://304/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
        new OpenCallback(NORMAL, "31m", "31d", function(entry) {
          mc.fired();
        })
      );
    })
  );

  do_test_pending();
}
