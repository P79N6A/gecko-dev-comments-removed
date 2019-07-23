







































function test() {
  
  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  const TEST_URL = "https://example.com/";

  
  gBrowser.selectedTab = gBrowser.addTab();
  let browser = gBrowser.selectedBrowser;
  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);

    pb.privateBrowsingEnabled = true;
    pb.privateBrowsingEnabled = false;

    
    browser.addEventListener("load", function() {
      browser.removeEventListener("load", arguments.callee, true);

      is(content.location, TEST_URL,
        "The original SSL page should be loaded at this stage");

      gBrowser.removeCurrentTab();
      gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
      finish();
    }, true);

    executeSoon(function () {
      content.location = TEST_URL;
    });
  }, true);
  content.location = TEST_URL;

  waitForExplicitFinish();
}
