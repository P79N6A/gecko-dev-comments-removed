






































function run_test() {
  do_check_eq(__LOCATION__.leafName, "test_location.js");
  
  do_import_script("tools/test-harness/xpcshell-simple/example/location_load.js");
}
