



function run_test() {
  
  do_test_pending();
  add_test(testStartupEnabled);
  add_test(testDisableAfterStartup);
  do_initialize_social(true, run_next_test);
}

function testStartupEnabled() {
  
  do_check_eq(Social.providers.length, 2, "two social providers enabled");
  do_check_true(Social.providers[0].enabled, "provider 0 is enabled");
  do_check_true(Social.providers[1].enabled, "provider 1 is enabled");
  run_next_test();
}

function testDisableAfterStartup() {
  let SocialService = Cu.import("resource://gre/modules/SocialService.jsm", {}).SocialService;
  SocialService.disableProvider(Social.providers[0].origin, function() {
    do_wait_observer("social:providers-changed", function() {
      do_check_eq(Social.enabled, false, "Social is disabled");
      do_check_eq(Social.providers.length, 0, "no social providers available");
      do_test_finished();
      run_next_test();
    });
    SocialService.disableProvider(Social.providers[0].origin)
  });
}
