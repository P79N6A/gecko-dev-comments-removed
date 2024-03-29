function run_test()
{
  if (!("@mozilla.org/toolkit/crash-reporter;1" in Components.classes)) {
    dump("INFO | test_crash_after_js_oom_reported_2.js | Can't test crashreporter in a non-libxul build.\n");
    return;
  }

  do_crash(
    function() {
      crashType = CrashTestUtils.CRASH_MOZ_CRASH;
      crashReporter.annotateCrashReport("TestingOOMCrash", "Yes");
      Components.utils.getJSTestingFunctions().reportOutOfMemory();
      Components.utils.forceGC();  
      Components.utils.getJSTestingFunctions().reportOutOfMemory();
    },
    function(mdump, extra) {
      do_check_eq(extra.TestingOOMCrash, "Yes");

      
      
      
      
      do_check_eq(extra.JSOutOfMemory, "Reported");
    },
    true);
}
