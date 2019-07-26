




const Cc = Components.classes;
const Ci = Components.interfaces;

function run_test() {
  do_get_profile_startup();

  var plugin = get_test_plugintag();
  do_check_true(plugin == null);

  
  do_get_profile();

  var plugin = get_test_plugintag();
  do_check_false(plugin == null);
}
