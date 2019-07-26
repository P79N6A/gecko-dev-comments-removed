function run_test()
{
  
  
  do_crash(
    function() {
        CrashTestUtils.TryOverrideExceptionHandler();
    },
    function(mdump, extra) {
    },
    true);
}
