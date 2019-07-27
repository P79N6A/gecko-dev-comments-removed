





add_task(function* () {
  var tab1 = gBrowser.addTab();
  var tab2 = gBrowser.addTab();

  
  
  yield BrowserTestUtils.switchTab(gBrowser, tab2);
  gURLBar.focus(); 
  yield BrowserTestUtils.switchTab(gBrowser, tab1);
  gURLBar.openPopup();
  yield BrowserTestUtils.switchTab(gBrowser, tab2);
  ok(!gURLBar.popupOpen, "urlbar focused in tab to switch to, close popup");
  
  
  gBrowser.removeCurrentTab();
  gBrowser.removeCurrentTab();
});
