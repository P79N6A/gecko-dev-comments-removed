





var gManagerWindow;

function test() {
  waitForExplicitFinish();

  open_manager(null, function(aWindow) {
    gManagerWindow = aWindow;
    run_next_test();
  });
}

function end_test() {
  close_manager(gManagerWindow, function() {
    finish();
  });
}

add_test(function() {
  var menuitem = gManagerWindow.document.getElementById("utils-backgroudUpdateCheck");

  function is_backgroundcheck_insync(aExpected) {
    var enabled = Services.prefs.getBoolPref("extensions.update.enabled");
    var checked = menuitem.getAttribute("checked") == "true";
    is(enabled, aExpected, "Background check should be " + (aExpected ? "enabled" : "disabled"));
    is(checked, enabled, "Background check menuitem should be in sync with preference");
  }

  is_backgroundcheck_insync(true);
  info("Setting background check pref to true");
  Services.prefs.setBoolPref("extensions.update.enabled", false);
  is_backgroundcheck_insync(false);
  info("Setting background check pref to false");
  Services.prefs.setBoolPref("extensions.update.enabled", true);
  is_backgroundcheck_insync(true);
















});
