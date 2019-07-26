



function test() {
  waitForExplicitFinish();

  function testState(aPinned) {
    function elemAttr(id, attr) document.getElementById(id).getAttribute(attr);

    if (aPinned) {
      is(elemAttr("key_close", "disabled"), "true",
         "key_close should be disabled when a pinned-tab is selected");
      is(elemAttr("menu_close", "key"), "",
         "menu_close shouldn't have a key set when a pinned is selected");
    }
    else {
      is(elemAttr("key_close", "disabled"), "",
         "key_closed shouldn't have disabled state set when a non-pinned tab is selected");
      is(elemAttr("menu_close", "key"), "key_close",
         "menu_close should have key_close set as its key when a non-pinned tab is selected");
    }
  }

  let lastSelectedTab = gBrowser.selectedTab;
  ok(!lastSelectedTab.pinned, "We should have started with a regular tab selected");

  testState(false);

  let pinnedTab = gBrowser.addTab("about:blank");
  gBrowser.pinTab(pinnedTab);

  
  testState(false);

  
  gBrowser.selectedTab = pinnedTab;
  testState(true);

  gBrowser.selectedTab = lastSelectedTab;
  testState(false);
  
  gBrowser.selectedTab = pinnedTab;
  testState(true);

  
  gBrowser.unpinTab(pinnedTab);
  testState(false);

  gBrowser.pinTab(pinnedTab);  
  testState(true);

  
  gBrowser.removeTab(pinnedTab);
  testState(false);

  finish();
}
