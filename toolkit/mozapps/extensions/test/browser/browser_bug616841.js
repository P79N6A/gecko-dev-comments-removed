



function test_string_compare() {
  ok("C".localeCompare("D") < 0, "C < D");
  ok("D".localeCompare("C") > 0, "D > C");
  ok("\u010C".localeCompare("D") < 0, "\u010C < D");
  ok("D".localeCompare("\u010C") > 0, "D > \u010C");
}

function test() {
  waitForExplicitFinish();

  test_string_compare();

  AddonManager.getAddonByID("foo", function(aAddon) {
    test_string_compare();
    finish();
  });
}
