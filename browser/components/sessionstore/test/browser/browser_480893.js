



































function test() {
  

  waitForExplicitFinish();

  
  
  gPrefService.setIntPref("browser.startup.page", 0);
  let tab = gBrowser.addTab("about:sessionrestore");
  gBrowser.selectedTab = tab;
  let browser = tab.linkedBrowser;
  browser.addEventListener("load", function(aEvent) {
    this.removeEventListener("load", arguments.callee, true);
    let doc = browser.contentDocument;

    
    doc.getElementById("errorCancel").click();
    browser.addEventListener("load", function(aEvent) {
      this.removeEventListener("load", arguments.callee, true);
      let doc = browser.contentDocument;

      is(doc.URL, "about:blank", "loaded page is about:blank");

      
      
      
      gPrefService.setIntPref("browser.startup.page", 1);
      gBrowser.loadURI("about:sessionrestore");
      browser.addEventListener("load", function(aEvent) {
        this.removeEventListener("load", arguments.callee, true);
        let doc = browser.contentDocument;

        
        doc.getElementById("errorCancel").click();
        browser.addEventListener("load", function(aEvent) {
          this.removeEventListener("load", arguments.callee, true);
          let doc = browser.contentDocument;

          isnot(doc.URL, "about:sessionrestore", "something else than about:sessionrestore has been loaded");
          isnot(doc.URL, "about:blank", "something else than about:blank has been loaded");

          
          gBrowser.removeTab(tab);
          gPrefService.clearUserPref("browser.startup.page"); 
          finish();
        }, true);
      }, true);
    }, true);
  }, true);
}
