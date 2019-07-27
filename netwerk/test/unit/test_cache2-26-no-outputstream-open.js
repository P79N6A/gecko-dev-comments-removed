function run_test()
{
  do_get_profile();

  if (!newCacheBackEndUsed()) {
    do_check_true(true, "This test doesn't run when the old cache back end is used since the behavior is different");
    return;
  }

  
  asyncOpenCacheEntry("http://no-data/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
    new OpenCallback(NEW|METAONLY|DONTSETVALID|WAITFORWRITE, "meta", "", function(entry) {
      
      do_execute_soon(() => {
        Cu.forceGC(); 

        asyncOpenCacheEntry("http://no-data/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
          new OpenCallback(NORMAL, "meta", "", function(entry) {
            finish_cache2_test();
          })
        );
      });
    })
  );

  do_test_pending();
}
