



function test() {
  waitForExplicitFinish();

  let testURL = "http://example.org/browser/browser/base/content/test/general/dummy_page.html";
  let tabSelected = false;

  
  let baseTab = gBrowser.addTab(testURL);
  baseTab.linkedBrowser.addEventListener("load", function() {
    
    if (baseTab.linkedBrowser.currentURI.spec == "about:blank")
      return;
    baseTab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    let testTab = gBrowser.addTab();

    
    gBrowser.selectedTab = testTab;

    
    ok(testTab.linkedBrowser.sessionHistory.count < 2,
       "The test tab has 1 or less history entries");
    
    is(testTab.linkedBrowser.currentURI.spec, "about:blank",
       "The test tab is on about:blank");
    
    ok(!testTab.linkedBrowser.contentDocument.body.hasChildNodes(),
       "The test tab has no child nodes");
    ok(!testTab.hasAttribute("busy"),
       "The test tab doesn't have the busy attribute");

    
    gURLBar.value = "moz-action:switchtab," + testURL;
    
    gURLBar.focus();

    
    function onTabClose(aEvent) {
      gBrowser.tabContainer.removeEventListener("TabClose", onTabClose, false);
      
      is(aEvent.originalTarget, testTab, "Got the TabClose event for the right tab");
      
      ok(tabSelected, "Confirming that the tab was selected");
      gBrowser.removeTab(baseTab);
      finish();
    }
    function onTabSelect(aEvent) {
      gBrowser.tabContainer.removeEventListener("TabSelect", onTabSelect, false);
      
      is(aEvent.originalTarget, baseTab, "Got the TabSelect event for the right tab");
      
      is(gBrowser.selectedTab, baseTab, "We've switched to the correct tab");
      tabSelected = true;
    }

    
    gBrowser.tabContainer.addEventListener("TabClose", onTabClose, false);
    gBrowser.tabContainer.addEventListener("TabSelect", onTabSelect, false);

    
    EventUtils.synthesizeKey("VK_RETURN", {});
  }, true);
}

