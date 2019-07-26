



XPCOMUtils.defineLazyModuleGetter(this, "MozLoopService",
                                  "resource:///modules/loop/MozLoopService.jsm");

do_register_cleanup(function() {
  Services.prefs.clearUserPref("loop.do_not_disturb");
});

function test_get_do_not_disturb() {
  Services.prefs.setBoolPref("loop.do_not_disturb", false);

  do_check_false(MozLoopService.doNotDisturb);

  Services.prefs.setBoolPref("loop.do_not_disturb", true);

  do_check_true(MozLoopService.doNotDisturb);
}

function test_set_do_not_disturb() {
  Services.prefs.setBoolPref("loop.do_not_disturb", false);

  MozLoopService.doNotDisturb = true;

  do_check_true(Services.prefs.getBoolPref("loop.do_not_disturb"));
}

function run_test()
{
  test_get_do_not_disturb();
  test_set_do_not_disturb();
}
