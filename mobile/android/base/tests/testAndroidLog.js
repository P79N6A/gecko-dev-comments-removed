




add_task(function test_AndroidLog() {
  Components.utils.import("resource://gre/modules/AndroidLog.jsm");

  do_check_true(!!AndroidLog);

  do_check_true("v" in AndroidLog && typeof AndroidLog.v == "function");
  do_check_true("d" in AndroidLog && typeof AndroidLog.d == "function");
  do_check_true("i" in AndroidLog && typeof AndroidLog.i == "function");
  do_check_true("w" in AndroidLog && typeof AndroidLog.w == "function");
  do_check_true("e" in AndroidLog && typeof AndroidLog.e == "function");

  
  
  
  
  do_check_eq(48, AndroidLog.v("AndroidLogTest", "This is a verbose message."));
  do_check_eq(46, AndroidLog.d("AndroidLogTest", "This is a debug message."));
  do_check_eq(46, AndroidLog.i("AndroidLogTest", "This is an info message."));
  do_check_eq(48, AndroidLog.w("AndroidLogTest", "This is a warning message."));
  do_check_eq(47, AndroidLog.e("AndroidLogTest", "This is an error message."));

  
  do_check_eq(48, AndroidLog.v.bind(null, "AndroidLogTest")("This is a verbose message."));
  do_check_eq(46, AndroidLog.d.bind(null, "AndroidLogTest")("This is a debug message."));
  do_check_eq(46, AndroidLog.i.bind(null, "AndroidLogTest")("This is an info message."));
  do_check_eq(48, AndroidLog.w.bind(null, "AndroidLogTest")("This is a warning message."));
  do_check_eq(47, AndroidLog.e.bind(null, "AndroidLogTest")("This is an error message."));

  
  
});

run_next_test();
