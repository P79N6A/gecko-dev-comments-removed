











































function run_test() {
  do_check_true(typeof(this['MODULE_IMPORTED']) == "undefined");
  Components.utils.import("resource://test/import_module.jsm");
  do_check_true(MODULE_IMPORTED);
}
