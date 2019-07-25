



































function test() {
  
  let blanktab = gBrowser.addTab();
  gBrowser.selectedTab = blanktab;
  BrowserOpenAddonsMgr();

  is(blanktab, gBrowser.selectedTab, "Current tab should be blank tab");
  
  waitForExplicitFinish();
  gBrowser.selectedBrowser.addEventListener("load", function() {
    let browser = blanktab.linkedBrowser;
    is(browser.currentURI.spec, "about:addons", "about:addons should load into blank tab.");
    gBrowser.removeTab(blanktab);
    finish();
  }, true);
}
