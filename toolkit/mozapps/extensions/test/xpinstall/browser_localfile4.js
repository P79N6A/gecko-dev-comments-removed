


function test() {
  Harness.installBlockedCallback = allow_blocked;
  Harness.installsCompletedCallback = finish_test;
  Harness.setup();

  
  Services.prefs.setBoolPref("xpinstall.whitelist.fileRequest", false);

  var cr = Components.classes["@mozilla.org/chrome/chrome-registry;1"]
                     .getService(Components.interfaces.nsIChromeRegistry);

  var chromeroot = extractChromeRoot(gTestPath);
  try {
    var xpipath = cr.convertChromeURL(makeURI(chromeroot)).spec;
  } catch (ex) {
    var xpipath = chromeroot; 
  }
  var triggers = encodeURIComponent(JSON.stringify({
    "Unsigned XPI": TESTROOT + "unsigned.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(xpipath + "installtrigger.html?" + triggers);
}

function allow_blocked(installInfo) {
  ok(true, "Seen blocked");
  return false;
}

function finish_test(count) {
  is(count, 0, "No add-ons should have been installed");

  Services.prefs.clearUserPref("xpinstall.whitelist.fileRequest");

  gBrowser.removeCurrentTab();
  Harness.finish();
}
