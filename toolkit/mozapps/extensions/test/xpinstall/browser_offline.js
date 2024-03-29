let proxyPrefValue;



function test() {
  Harness.downloadProgressCallback = download_progress;
  Harness.installsCompletedCallback = finish_test;
  Harness.setup();

  var pm = Services.perms;
  pm.add(makeURI("http://example.com/"), "install", pm.ALLOW_ACTION);

  var triggers = encodeURIComponent(JSON.stringify({
    "Unsigned XPI": TESTROOT + "unsigned.xpi"
  }));
  gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.loadURI(TESTROOT + "installtrigger.html?" + triggers);
}

function download_progress(addon, value, maxValue) {
  try {
    
    
    proxyPrefValue = Services.prefs.getIntPref("network.proxy.type");
    Services.prefs.setIntPref("network.proxy.type", 0);
    Services.io.manageOfflineStatus = false;
    Services.io.offline = true;
  } catch (ex) {
  }
}

function finish_test(count) {
  function wait_for_online() {
    info("Checking if the browser is still offline...");

    let tab = gBrowser.selectedTab;
    tab.linkedBrowser.addEventListener("DOMContentLoaded", function errorLoad() {
      tab.linkedBrowser.removeEventListener("DOMContentLoaded", errorLoad, true);
      let url = tab.linkedBrowser.contentDocument.documentURI;
      info("loaded: " + url);
      if (/^about:neterror\?e=netOffline/.test(url)) {
        wait_for_online();
      } else {
        gBrowser.removeCurrentTab();
        Harness.finish();
      }
    }, true);
    tab.linkedBrowser.loadURI("http://example.com/");
  }

  is(count, 0, "No add-ons should have been installed");
  try {
    Services.prefs.setIntPref("network.proxy.type", proxyPrefValue);
    Services.io.offline = false;
  } catch (ex) {
  }

  Services.perms.remove(makeURI("http://example.com"), "install");

  wait_for_online();
}
