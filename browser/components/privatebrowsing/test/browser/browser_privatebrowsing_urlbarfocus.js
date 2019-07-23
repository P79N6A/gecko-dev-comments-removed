






































function test() {
  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  const kTestURL = "data:text/plain,test";
  let tab = gBrowser.addTab();
  gBrowser.selectedTab = tab;
  let browser = gBrowser.getBrowserForTab(tab);
  browser.addEventListener("load", function() {
    browser.removeEventListener("load", arguments.callee, true);

    
    browser.focus();
    isnot(document.commandDispatcher.focusedElement, gURLBar.inputField,
      "URL Bar should not be focused before entering the private browsing mode");
    
    isnot(gURLBar.value, "", "URL Bar should no longer be empty after leaving the private browsing mode");

    
    pb.privateBrowsingEnabled = true;
    tab = gBrowser.selectedTab;
    browser = gBrowser.getBrowserForTab(tab);
    browser.addEventListener("load", function() {
      
      setTimeout(function() {
        
        is(document.commandDispatcher.focusedElement, gURLBar.inputField,
          "URL Bar should be focused inside the private browsing mode");
        
        is(gURLBar.value, "", "URL Bar should be empty inside the private browsing mode");

        
        pb.privateBrowsingEnabled = false;
        tab = gBrowser.selectedTab;
        browser = gBrowser.getBrowserForTab(tab);
        browser.addEventListener("load", function() {
          
          isnot(document.commandDispatcher.focusedElement, gURLBar.inputField,
            "URL Bar should no longer be focused after leaving the private browsing mode");
          
          isnot(gURLBar.value, "", "URL Bar should no longer be empty after leaving the private browsing mode");

          gBrowser.removeTab(tab);
          finish();
        }, true);
      }, 0);
    }, true);
  }, true);
  browser.contentWindow.location = kTestURL;

  waitForExplicitFinish();
}
