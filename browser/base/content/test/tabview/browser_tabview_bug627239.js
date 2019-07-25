




































function test() {
  waitForExplicitFinish();
  
  
  
  Services.prefs.setIntPref("browser.sessionstore.max_concurrent_tabs", 0);
  
  newWindowWithTabView(onTabViewWindowLoaded);
}

registerCleanupFunction(function() {
  Services.prefs.clearUserPref("browser.sessionstore.max_concurrent_tabs");
})

function onTabViewWindowLoaded(win) {
  ok(win.TabView.isVisible(), "Tab View is visible");

  let contentWindow = win.document.getElementById("tab-view").contentWindow;
  let [originalTab] = win.gBrowser.visibleTabs;
  let originalGroup = contentWindow.GroupItems.getActiveGroupItem();
  let originalURL   = contentWindow.TabUtils.URLOf(originalTab);

  
  let insecure = win.gBrowser.loadOneTab("http://example.com", {inBackground: true});
  let secure   = win.gBrowser.loadOneTab("https://example.com", {inBackground: true});
  
  
  let restoredWindow;
  
  function checkState() {
    is(win.gBrowser.tabs.length, 3, "There are three tabs");
    is(win.gBrowser.visibleTabs.length, 3, "All are visible");
    is(contentWindow.UI.getActiveTab(), originalTab._tabViewTabItem,
      "The original tab is active (Panorama)");
    is(win.gBrowser.selectedTab, originalTab,
      "The original tab is active (tabbrowser)");

    
    contentWindow.TabItems.saveAll(true);
    ok(contentWindow.Storage.getTabData(insecure).imageData,
      "Insecure tab has imageData");
    ok(!contentWindow.Storage.getTabData(secure).imageData,
      "Secure tab doesn't have imageData");

    Services.obs.addObserver(
      xulWindowDestroy, "xul-window-destroyed", false);
    win.close();
  }

  function xulWindowDestroy() {
    Services.obs.removeObserver(
       xulWindowDestroy, "xul-window-destroyed", false);
    executeSoon(function() {
      restoredWindow = undoCloseWindow(0);
      restoredWindow.addEventListener("load", function() {
        restoredWindow.removeEventListener("load", arguments.callee, false);
        showTabView(checkRestored, restoredWindow);
      }, false);
    });
  }
  
  function checkRestored() {
    let contentWindow = restoredWindow.document.getElementById("tab-view").
                        contentWindow;
    let TabUtils = contentWindow.TabUtils;
    let tabs = restoredWindow.gBrowser.tabs;
    is(tabs.length, 3, "There are three tabs");
    is(restoredWindow.gBrowser.visibleTabs.length, 3, "All are visible");
    is(TabUtils.URLOf(restoredWindow.gBrowser.selectedTab), originalURL,
      "The original tab is active");

    afterAllTabItemsUpdated(function() {
      ok(tabs[1]._tabViewTabItem.isShowingCachedData(),
         "Insecure tab is showing cached data");
      ok(!tabs[2]._tabViewTabItem.isShowingCachedData(),
         "Secure tab is not showing cached data");
  
      restoredWindow.close();
      finish();
    }, restoredWindow);
  }
    
  afterAllTabsLoaded(function() {
    afterAllTabItemsUpdated(function () {
      hideTabView(checkState, win);
    }, win);
  }, win);
}
