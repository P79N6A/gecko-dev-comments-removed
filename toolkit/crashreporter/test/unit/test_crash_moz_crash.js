function run_test()
{
  
  do_crash(function() {
             crashType = CrashTestUtils.CRASH_MOZ_CRASH;
             crashReporter.annotateCrashReport("TestKey", "TestValue");
           },
           function(mdump, extra) {
             do_check_eq(extra.TestKey, "TestValue");
             do_check_false("OOMAllocationSize" in extra);
           },
          
          true);
}
