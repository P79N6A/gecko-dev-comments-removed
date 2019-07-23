






































function run_test() {
  do_check_eq(__LOCATION__.leafName, "test_location.js");
  
  do_import_script("testing/xpcshell/example/location_load.js");
}
