



function run_test() {
  
  
  var profilerCc = Cc["@mozilla.org/tools/profiler;1"];
  if (!profilerCc)
    return;

  var profiler = Cc["@mozilla.org/tools/profiler;1"].getService(Ci.nsIProfiler);
  if (!profiler)
    return;

  var sharedStr = profiler.getSharedLibraryInformation();
  sharedStr = sharedStr.toLowerCase();

  
  
  do_check_neq(sharedStr, null);
  do_check_neq(sharedStr, "");
}
