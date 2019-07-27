



function test() {
  Harness.installBlockedCallback = allow_blocked;
  Harness.installsCompletedCallback = finish_test;
  Harness.setup();

  
  Services.prefs.setBoolPref("xpinstall.whitelist.directRequest", false);

  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "unsigned.xpi");
}

function allow_blocked(installInfo) {
  ok(true, "Seen blocked");
  return false;
}

function finish_test(count) {
  is(count, 0, "No add-ons should have been installed");

  Services.perms.remove(makeURI("http://example.org"), "install");
  Services.prefs.clearUserPref("xpinstall.whitelist.directRequest");

  gBrowser.removeCurrentTab();
  Harness.finish();
}
