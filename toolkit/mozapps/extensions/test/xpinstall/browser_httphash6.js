


function setup_redirect(aSettings) {
  var url = "https://example.com/browser/" + RELATIVE_DIR + "redirect.sjs?mode=setup";
  for (var name in aSettings) {
    url += "&" + name + "=" + aSettings[name];
  }

  var req = new XMLHttpRequest();
  req.open("GET", url, false);
  req.send(null);
}

var gInstall = null;

function test() {
  Harness.downloadFailedCallback = download_failed;
  Harness.installsCompletedCallback = finish_failed_download;
  Harness.setup();

  var pm = Services.perms;
  pm.add(makeURI("http://example.com/"), "install", pm.ALLOW_ACTION);
  Services.prefs.setBoolPref(PREF_INSTALL_REQUIREBUILTINCERTS, false);

  
  setup_redirect({
    "X-Target-Digest": "sha1:foo",
    "Location": "http://example.com/browser/" + RELATIVE_DIR + "unsigned.xpi"
  });

  var url = "https://example.com/browser/" + RELATIVE_DIR + "redirect.sjs?mode=redirect";

  var triggers = encodeURIComponent(JSON.stringify({
    "Unsigned XPI": {
      URL: url,
      toString: function() { return this.URL; }
    }
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
}

function download_failed(install) {
  is(install.error, AddonManager.ERROR_INCORRECT_HASH, "Should have seen a hash failure");
  
  gInstall = install;
}

function finish_failed_download() {
  
  Harness.installEndedCallback = install_ended;
  Harness.installsCompletedCallback = finish_test;
  Harness.setup();

  
  setup_redirect({
    "X-Target-Digest": "sha1:3d0dc22e1f394e159b08aaf5f0f97de4d5c65f4f",
    "Location": "http://example.com/browser/" + RELATIVE_DIR + "unsigned.xpi"
  });

  
  Harness.onNewInstall(gInstall);

  
  AddonManager.installAddonsFromWebpage("application/x-xpinstall",
                                        gBrowser.contentWindow,
                                        gBrowser.currentURI, [gInstall]);
}

function install_ended(install, addon) {
  install.cancel();
}

function finish_test(count) {
  is(count, 1, "1 Add-on should have been successfully installed");

  Services.perms.remove("example.com", "install");
  Services.prefs.clearUserPref(PREF_INSTALL_REQUIREBUILTINCERTS);

  gBrowser.removeCurrentTab();
  Harness.finish();
}
