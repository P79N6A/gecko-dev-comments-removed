



































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

      
      
      let homepage = "http://localhost:8888/";
      gPrefService.setCharPref("browser.startup.homepage", homepage);
      gPrefService.setIntPref("browser.startup.page", 1);
      gBrowser.loadURI("about:sessionrestore");
      browser.addEventListener("load", function(aEvent) {
        this.removeEventListener("load", arguments.callee, true);
        let doc = browser.contentDocument;

        
        doc.getElementById("errorCancel").click();
        browser.addEventListener("load", function(aEvent) {
          this.removeEventListener("load", arguments.callee, true);
          let doc = browser.contentDocument;

          is(doc.URL, homepage, "loaded page is the homepage");

          
          gBrowser.removeTab(tab);
          
          
          if (gPrefService.prefHasUserValue("browser.startup.page"))
            gPrefService.clearUserPref("browser.startup.page");
          gPrefService.clearUserPref("browser.startup.homepage");
          finish();
        }, true);
      }, true);
    }, true);
  }, true);
}
