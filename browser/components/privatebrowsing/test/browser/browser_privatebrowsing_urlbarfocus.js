






































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  const TEST_URL = "data:text/plain,test";
  gBrowser.selectedTab = gBrowser.addTab();
  let browser = gBrowser.selectedBrowser;
  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);

    
    browser.focus();
    isnot(document.commandDispatcher.focusedElement, gURLBar.inputField,
      "URL Bar should not be focused before entering the private browsing mode");
    
    isnot(gURLBar.value, "", "URL Bar should no longer be empty after leaving the private browsing mode");

    
    pb.privateBrowsingEnabled = true;
    browser = gBrowser.selectedBrowser;
    browser.addEventListener("load", function() {
      
      setTimeout(function() {
        
        is(document.commandDispatcher.focusedElement, gURLBar.inputField,
          "URL Bar should be focused inside the private browsing mode");
        
        is(gURLBar.value, "", "URL Bar should be empty inside the private browsing mode");

        
        pb.privateBrowsingEnabled = false;
        browser = gBrowser.selectedBrowser;
        browser.addEventListener("load", function() {
          
          isnot(document.commandDispatcher.focusedElement, gURLBar.inputField,
            "URL Bar should no longer be focused after leaving the private browsing mode");
          
          isnot(gURLBar.value, "", "URL Bar should no longer be empty after leaving the private browsing mode");

          gBrowser.removeCurrentTab();
          finish();
        }, true);
      }, 0);
    }, true);
  }, true);
  content.location = TEST_URL;

  waitForExplicitFinish();
}
