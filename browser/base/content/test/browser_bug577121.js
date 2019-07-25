



































function test() {
  
  
  let testTab1 = gBrowser.addTab();
  let testTab2 = gBrowser.addTab();
  gBrowser.pinTab(testTab2);

  
  
  
  gBrowser.removeAllTabsBut(testTab1);

  is(gBrowser.tabs.length, 2, "there are two remaining tabs open");
  is(gBrowser.tabs[0], testTab2, "pinned tab2 stayed open");
  is(gBrowser.tabs[1], testTab1, "tab1 stayed open");
  
  
  
  gBrowser.removeTab(testTab2);
}
