





function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  window.addEventListener("DOMContentLoaded", checkPage, false);

  
  Services.io.offline = true;
  gPrefService.setBoolPref("browser.cache.disk.enable", false);
  gPrefService.setBoolPref("browser.cache.memory.enable", false);
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

  ok(!Services.io.offline, "After clicking the Try Again button, we're back "
   +" online. This depends on Components.interfaces.nsIDOMWindowUtils being "
   +"available from untrusted content (bug 435325).");

  finish();
}

registerCleanupFunction(function() {
  gPrefService.setBoolPref("browser.cache.disk.enable", true);
  gPrefService.setBoolPref("browser.cache.memory.enable", true);
  Services.io.offline = false;
  gBrowser.removeCurrentTab();
});
