



function test() {
  

  waitForExplicitFinish();

  
  
  gPrefService.setIntPref("browser.startup.page", 0);
  let tab = gBrowser.addTab("about:sessionrestore");
  gBrowser.selectedTab = tab;
  let browser = tab.linkedBrowser;
  promiseBrowserLoaded(browser).then(() => {
    let doc = browser.contentDocument;

    
    doc.getElementById("errorCancel").click();
    promiseBrowserLoaded(browser).then(() => {
      let doc = browser.contentDocument;

      is(doc.URL, "about:blank", "loaded page is about:blank");

      
      
      let homepage = "http://mochi.test:8888/";
      gPrefService.setCharPref("browser.startup.homepage", homepage);
      gPrefService.setIntPref("browser.startup.page", 1);
      gBrowser.loadURI("about:sessionrestore");
      promiseBrowserLoaded(browser).then(() => {
        let doc = browser.contentDocument;

        
        doc.getElementById("errorCancel").click();
        promiseBrowserLoaded(browser).then(() => {
          let doc = browser.contentDocument;

          is(doc.URL, homepage, "loaded page is the homepage");

          
          gBrowser.removeTab(tab);
          gPrefService.clearUserPref("browser.startup.page");
          gPrefService.clearUserPref("browser.startup.homepage");
          finish();
        });
      });
    });
  });
}
