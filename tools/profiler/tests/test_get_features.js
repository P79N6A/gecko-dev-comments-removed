


  
function run_test() {
  
  
  var profilerCc = Cc["@mozilla.org/tools/profiler;1"];
  if (!profilerCc)
    return;

  var profiler = Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);
  if (!profiler)
    return;

  var profilerFeatures = profiler.GetFeatures([]);
  do_check_true(profilerFeatures != null);
}
