





function test() {
  var tab1 = gBrowser.addTab();
  var tab2 = gBrowser.addTab();

  
  
  gBrowser.selectedTab = tab2;
  gURLBar.focus(); 
  gBrowser.selectedTab = tab1;
  gURLBar.openPopup();
  gBrowser.selectedTab = tab2;
  ok(!gURLBar.popupOpen, "urlbar focused in tab to switch to, close popup");
  
  
  gBrowser.removeCurrentTab();
  gBrowser.removeCurrentTab();
}
