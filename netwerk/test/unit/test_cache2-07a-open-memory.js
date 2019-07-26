function run_test()
{
  do_get_profile();

  if (!newCacheBackEndUsed()) {
    do_check_true(true, "This test doesn't run when the old cache back end is used since the behavior is different");
    return;
  }

  

  asyncOpenCacheEntry("http://mem-first/", "memory", Ci.nsICacheStorage.OPEN_NORMALLY, null,
    new OpenCallback(NEW, "mem1-meta", "mem1-data", function(entryM1) {
      do_check_false(entryM1.persistent);
      asyncOpenCacheEntry("http://mem-first/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
        new OpenCallback(NORMAL, "mem1-meta", "mem1-data", function(entryM2) {
          do_check_false(entryM1.persistent);
          do_check_false(entryM2.persistent);

          

          asyncOpenCacheEntry("http://disk-first/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
            
            
            
            new OpenCallback(NEW|WAITFORWRITE, "disk1-meta", "disk1-data", function(entryD1) {
              do_check_true(entryD1.persistent);
              
              asyncOpenCacheEntry("http://disk-first/", "memory", Ci.nsICacheStorage.OPEN_NORMALLY, null,
                
                new OpenCallback(NEW, "mem2-meta", "mem2-data", function(entryD2) {
                  do_check_true(entryD1.persistent);
                  do_check_false(entryD2.persistent);
                  
                  asyncOpenCacheEntry("http://disk-first/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, null,
                    new OpenCallback(NORMAL, "mem2-meta", "mem2-data", function(entryD3) {
                      do_check_true(entryD1.persistent);
                      do_check_false(entryD2.persistent);
                      do_check_false(entryD3.persistent);
                      finish_cache2_test();
                    })
                  );
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
