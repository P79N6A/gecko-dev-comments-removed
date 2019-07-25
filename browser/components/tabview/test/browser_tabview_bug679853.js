


function test() {
  waitForExplicitFinish();

  
  registerCleanupFunction(function () {
    while (gBrowser.tabs.length > 1)
      gBrowser.removeTab(gBrowser.tabs[1]);

    hideTabView();
  });

  
  gBrowser.selectedTab = gBrowser.addTab();

  showTabView(function () {
    
    togglePrivateBrowsing(function () {
      ok(!TabView.isVisible(), "tabview is hidden");

      showTabView(function () {
        
        togglePrivateBrowsing(function () {
          ok(TabView.isVisible(), "tabview is visible");
          hideTabView(finish);
        });
      });
    });
  });
}
