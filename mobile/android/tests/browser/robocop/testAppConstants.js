




Components.utils.import("resource://gre/modules/AppConstants.jsm");

add_task(function* testAppConstants() {
  let packageName = AppConstants.ANDROID_PACKAGE_NAME
  do_check_neq(packageName, "@ANDROID_PACKAGE_NAME@");
  do_check_true(packageName.length > 0);
});


run_next_test();
