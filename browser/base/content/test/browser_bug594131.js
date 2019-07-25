



function test() {
  let newTab = gBrowser.addTab("http://example.com");
  waitForExplicitFinish();
  newTab.linkedBrowser.addEventListener("load", mainPart, true);

  function mainPart() {
    newTab.linkedBrowser.removeEventListener("load", mainPart, true);

    gBrowser.pinTab(newTab);
    gBrowser.selectedTab = newTab;

    openUILinkIn("http://example.org/", "current", { inBackground: true });
    isnot(gBrowser.selectedTab, newTab, "shouldn't load in background");

    gBrowser.removeTab(newTab);
    gBrowser.removeTab(gBrowser.tabs[1]); 
    finish();
  }
}
