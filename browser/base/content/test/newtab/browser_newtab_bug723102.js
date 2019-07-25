


function runTests() {
  
  setLinks("0,1,2,3,4,5,6,7,8");
  setPinnedLinks("");

  yield addNewTabPageTab();
  let firstTab = gBrowser.selectedTab;

  yield addNewTabPageTab();
  gBrowser.removeTab(firstTab);

  ok(NewTabUtils.allPages.enabled, true, "page is enabled");
  NewTabUtils.allPages.enabled = false;
  ok(cw.gGrid.node.hasAttribute("page-disabled"), "page is disabled");
}
