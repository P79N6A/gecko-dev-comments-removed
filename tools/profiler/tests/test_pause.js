



function run_test() {
  
  
  var profilerCc = Cc["@mozilla.org/tools/profiler;1"];
  if (!profilerCc)
    return;

  var profiler = profilerCc.getService(Ci.nsIProfiler);
  if (!profiler)
    return;

  do_check_true(!profiler.IsActive());
  do_check_true(!profiler.IsPaused());

  profiler.StartProfiler(1000, 10, [], 0);

  do_check_true(profiler.IsActive());

  profiler.PauseSampling();

  do_check_true(profiler.IsPaused());

  profiler.ResumeSampling();

  do_check_true(!profiler.IsPaused());

  profiler.StopProfiler();
  do_check_true(!profiler.IsActive());
  do_check_true(!profiler.IsPaused());
  do_test_finished();
}
