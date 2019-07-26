



function run_test() {
  
  
  var profilerCc = Cc["@mozilla.org/tools/profiler;1"];
  if (!profilerCc)
    return;

  var profiler = Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);
  if (!profiler)
    return;

  do_check_true(!profiler.IsActive());

  profiler.StartProfiler(1000, 10, [], 0);

  do_check_true(profiler.IsActive());

  do_test_pending();

  do_timeout(1000, function wait() {
    
    var profileStr = profiler.GetProfile();
    do_check_true(profileStr.length > 10);

    
    var profileObj = profiler.getProfileData();
    do_check_neq(profileObj, null);
    do_check_neq(profileObj.threads, null);
    do_check_true(profileObj.threads.length >= 1);
    do_check_neq(profileObj.threads[0].samples, null);
    
    

    profiler.StopProfiler();
    do_check_true(!profiler.IsActive());
    do_test_finished();
  });


}
