


function run_test() {
  Components.utils.import("resource:///modules/LogCapture.jsm");
  run_next_test();
}


add_test(function test_logCapture_loads() {
  ok(LogCapture, "LogCapture object exists");
  run_next_test();
});
