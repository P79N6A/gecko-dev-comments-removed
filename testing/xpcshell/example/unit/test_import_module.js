












































function run_test() {
  do_check_true(typeof(this['MODULE_IMPORTED']) == "undefined");
  do_check_true(typeof(this['MODULE_URI']) == "undefined");
  let uri = "resource://test/import_module.jsm";
  Components.utils.import(uri);
  do_check_true(MODULE_URI == uri);
  do_check_true(MODULE_IMPORTED);
  do_check_true(SUBMODULE_IMPORTED);
  do_check_true(same_scope);
}
