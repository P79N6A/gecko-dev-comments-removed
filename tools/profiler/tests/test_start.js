


  
function run_test() {
  
  
  var profilerCc = Cc["@mozilla.org/tools/profiler;1"];
  if (!profilerCc)
    return;

  var profiler = Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);
  if (!profiler)
    return;

  do_check_true(!profiler.IsActive());

  profiler.StartProfiler(10, 100, [], 0);

  do_check_true(profiler.IsActive());

  profiler.StopProfiler();

  do_check_true(!profiler.IsActive());
}
