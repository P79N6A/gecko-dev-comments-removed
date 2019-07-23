







































function test() {
  
  let prefBranch = Cc["@mozilla.org/preferences-service;1"].
                   getService(Ci.nsIPrefBranch);
  prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  const kTestURL = "https://example.com/";

  
  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;
  let browser = gBrowser.getBrowserForTab(tab);
  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);

    pb.privateBrowsingEnabled = true;
    pb.privateBrowsingEnabled = false;

    
    browser.addEventListener("load", function() {
      browser.removeEventListener("load", arguments.callee, true);

      is(browser.contentWindow.location, kTestURL,
        "The original SSL page should be loaded at this stage");

      gBrowser.removeTab(tab);
      prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
      finish();
    }, true);

    executeSoon(function(){
      browser.contentWindow.location = kTestURL;
    });
  }, true);
  browser.contentWindow.location = kTestURL;

  waitForExplicitFinish();
}
