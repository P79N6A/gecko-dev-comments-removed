



































function test() {
  let tab1 = gBrowser.selectedTab;
  let tab2 = gBrowser.addTab("about:blank", {skipAnimation: true});
  let tab3 = gBrowser.addTab();
  gBrowser.selectedTab = tab2;
  
  gBrowser.removeCurrentTab({animate: true});
  gBrowser.tabContainer.advanceSelectedTab(-1, true);
  is(gBrowser.selectedTab, tab1, "First tab should be selected");
  gBrowser.removeTab(tab2);
  
  
  gBrowser.removeCurrentTab({animate: true});
  try {
    gBrowser.tabContainer.advanceSelectedTab(-1, false);
  } catch(err) {
    ok(false, "Shouldn't throw");
  }
  
  gBrowser.removeTab(tab1);
}
