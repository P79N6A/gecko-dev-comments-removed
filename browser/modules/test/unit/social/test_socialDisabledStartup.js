



function run_test() {
  
  Services.prefs.setBoolPref("social.allowMultipleWorkers", true);
  do_register_cleanup(function() {
    Services.prefs.clearUserPref("social.allowMultipleWorkers");
  });
  do_test_pending();
  add_test(testStartupDisabled);
  add_test(testEnableAfterStartup);
  do_initialize_social(false, run_next_test);
}

function testStartupDisabled() {
  
  do_check_eq(Social.providers.length, 2, "two social providers available");
  do_check_false(Social.providers[0].enabled, "provider is enabled");
  do_check_false(Social.providers[1].enabled, "provider is enabled");
  run_next_test();
}

function testEnableAfterStartup() {
  do_wait_observer("social:provider-set", function() {
    do_check_true(Social.enabled, "Social is enabled");
    do_check_eq(Social.providers.length, 2, "two social providers available");
    do_check_true(Social.providers[0].enabled, "provider is enabled");
    do_check_true(Social.providers[1].enabled, "provider is enabled");
    do_test_finished();
    run_next_test();
  });
  Social.enabled = true;
}
