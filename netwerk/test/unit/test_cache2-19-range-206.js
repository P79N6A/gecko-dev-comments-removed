function run_test()
{
  if (!newCacheBackEndUsed()) {
    do_check_true(true, "This test doesn't run when the old cache back end is used since the behavior is different");
    return;
  }

  do_get_profile();

  
  asyncOpenCacheEntry("http://r206/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
    new OpenCallback(NEW, "206m", "206part1-", function(entry) {
      
      asyncOpenCacheEntry("http://r206/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
        new OpenCallback(PARTIAL, "206m", "206part1-", function(entry) {
          
          (new OpenCallback(NEW|WAITFORWRITE|PARTIAL, "206m", "-part2", function(entry) {
            entry.setValid();
          })).onCacheEntryAvailable(entry, true, null, Cr.NS_OK);
        })
      );

      var mc = new MultipleCallbacks(3, finish_cache2_test);

      asyncOpenCacheEntry("http://r206/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
        new OpenCallback(NORMAL, "206m", "206part1--part2", function(entry) {
          mc.fired();
        })
      );
      asyncOpenCacheEntry("http://r206/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
        new OpenCallback(NORMAL, "206m", "206part1--part2", function(entry) {
          mc.fired();
        })
      );
      asyncOpenCacheEntry("http://r206/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
        new OpenCallback(NORMAL, "206m", "206part1--part2", function(entry) {
          mc.fired();
        })
      );
    })
  );

  do_test_pending();
}
