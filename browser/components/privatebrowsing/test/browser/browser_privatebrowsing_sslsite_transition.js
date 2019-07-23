







































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  const kTestURL = "https://example.com/";

  
  let tab = gBrowser.tabContainer.childNodes[0];
  let browser = gBrowser.getBrowserForTab(tab);
  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);

    
    pb.privateBrowsingEnabled = true;
    tab = gBrowser.tabContainer.childNodes[0];
    browser = gBrowser.getBrowserForTab(tab);
    browser.addEventListener("load", function() {
      browser.removeEventListener("load", arguments.callee, true);

      is(browser.contentWindow.location, "about:privatebrowsing",
        "about:privatebrowsing should be loaded at this stage");

      
      pb.privateBrowsingEnabled = false;
      tab = gBrowser.tabContainer.childNodes[0];
      browser = gBrowser.getBrowserForTab(tab);
      
      browser.addEventListener("load", function() {
        browser.removeEventListener("load", arguments.callee, true);

        is(browser.contentWindow.location, kTestURL,
          "The original SSL page should be loaded at this stage");

        browser.contentWindow.location = "about:blank";
        finish();
      }, true);
    }, true);
  }, true);
  browser.contentWindow.location = kTestURL;

  waitForExplicitFinish();
}
