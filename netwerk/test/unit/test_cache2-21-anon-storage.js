Components.utils.import('resource://gre/modules/LoadContextInfo.jsm');

function run_test()
{
  do_get_profile();

  if (!newCacheBackEndUsed()) {
    do_check_true(true, "This test doesn't run when the old cache back end is used since the behavior is different");
    return;
  }

  
  asyncOpenCacheEntry("http://anon1/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, LoadContextInfo.anonymous,
    new OpenCallback(NEW, "an1", "an1", function(entry) {
      asyncOpenCacheEntry("http://anon1/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, LoadContextInfo.anonymous,
        new OpenCallback(NORMAL, "an1", "an1", function(entry) {
          
          asyncOpenCacheEntry("http://anon1/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, LoadContextInfo.default,
            new OpenCallback(NEW, "na1", "na1", function(entry) {
              asyncOpenCacheEntry("http://anon1/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, LoadContextInfo.default,
                new OpenCallback(NORMAL, "na1", "na1", function(entry) {
                  
                  asyncOpenCacheEntry("http://anon1/", "disk", Ci.nsICacheStorage.OPEN_NORMALLY, LoadContextInfo.anonymous,
                    new OpenCallback(NORMAL, "an1", "an1", function(entry) {
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
