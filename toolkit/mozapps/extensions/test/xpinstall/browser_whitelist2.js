



function test() {
  Harness.installBlockedCallback = allow_blocked;
  Harness.installsCompletedCallback = finish_test;
  Harness.setup();

  var pm = Services.perms;
  pm.add(makeURI("http://example.org/"), "install", pm.ALLOW_ACTION);

  var triggers = encodeURIComponent(JSON.stringify({
    "Unsigned XPI": TESTROOT2 + "unsigned.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
}

function allow_blocked(installInfo) {
  is(installInfo.originatingWindow, gBrowser.contentWindow, "Install should have been triggered by the right window");
  is(installInfo.originatingURI.spec, gBrowser.currentURI.spec, "Install should have been triggered by the right uri");
  return false;
}

function finish_test() {
  Services.perms.remove("example.org", "install");

  gBrowser.removeCurrentTab();
  Harness.finish();
}
