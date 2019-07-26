




let proxyPrefValue;

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  window.addEventListener("DOMContentLoaded", checkPage, false);

  
  Services.io.offline = true;

  
  
  proxyPrefValue = Services.prefs.getIntPref("network.proxy.type");
  Services.prefs.setIntPref("network.proxy.type", 0);

  Services.prefs.setBoolPref("browser.cache.disk.enable", false);
  Services.prefs.setBoolPref("browser.cache.memory.enable", false);
  content.location = "http://example.com/";
}

function checkPage() {
  if(content.location == "about:blank") {
    info("got about:blank, which is expected once, so return");
    return;
  }

  window.removeEventListener("DOMContentLoaded", checkPage, false);

  ok(Services.io.offline, "Setting Services.io.offline to true.");
  is(gBrowser.contentDocument.documentURI.substring(0,27),
    "about:neterror?e=netOffline", "Loading the Offline mode neterror page.");

  
  ok(gBrowser.contentDocument.getElementById("errorTryAgain"),
    "The error page has got a #errorTryAgain element");
  gBrowser.contentDocument.getElementById("errorTryAgain").click();

  ok(!Services.io.offline, "After clicking the Try Again button, we're back " +
                           "online.");

  finish();
}

registerCleanupFunction(function() {
  Services.prefs.setIntPref("network.proxy.type", proxyPrefValue);
  Services.prefs.setBoolPref("browser.cache.disk.enable", true);
  Services.prefs.setBoolPref("browser.cache.memory.enable", true);
  Services.io.offline = false;
  gBrowser.removeCurrentTab();
});
