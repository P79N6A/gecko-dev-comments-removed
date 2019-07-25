






































function test() {
  waitForExplicitFinish();

  
  let testTab = gBrowser.addTab("about:blank", {skipAnimation: true});
  is(gBrowser.visibleTabs.length, 2, "just added a tab, so 2 tabs");
  gBrowser.selectedTab = testTab;

  let numVisBeforeHide, numVisAfterHide;
  gBrowser.tabContainer.addEventListener("TabSelect", function() {
    gBrowser.tabContainer.removeEventListener("TabSelect", arguments.callee, false);

    
    numVisBeforeHide = gBrowser.visibleTabs.length;
    gBrowser.hideTab(testTab);
    numVisAfterHide = gBrowser.visibleTabs.length;
  }, false);
  gBrowser.removeTab(testTab, {animate: true});

  
  (function checkRemoved() setTimeout(function() {
    if (gBrowser.tabs.length != 1)
      return checkRemoved();

    is(numVisBeforeHide, 1, "animated remove has in 1 tab left");
    is(numVisAfterHide, 1, "hiding a removing tab is also has 1 tab");
    finish();
  }, 50))();
}
