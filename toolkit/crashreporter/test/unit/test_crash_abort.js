


function run_test()
{
  
  do_crash(function() {
             crashType = CrashTestUtils.CRASH_ABORT;
             crashReporter.annotateCrashReport("TestKey", "TestValue");
           },
           function(mdump, extra) {
             do_check_eq(extra.TestKey, "TestValue");
           },
          
          true);
}
