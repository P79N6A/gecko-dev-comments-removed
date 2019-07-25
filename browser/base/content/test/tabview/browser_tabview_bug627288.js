


function test() {
  let cw;
  let tab;

  let testReconnectWithSameUrl = function () {
    tab = gBrowser.loadOneTab('http://mochi.test:8888/', {inBackground: true});

    afterAllTabsLoaded(function () {
      let tabItem = tab._tabViewTabItem;
      let data = tabItem.getStorageData(true);
      gBrowser.removeTab(tab);

      cw.TabItems.pauseReconnecting();
      tab = gBrowser.loadOneTab('http://mochi.test:8888/', {inBackground: true});
      cw.Storage.saveTab(tab, data);

      whenTabAttrModified(tab, function () {
        tabItem = tab._tabViewTabItem;

        
        
        tabItem.addSubscriber(tabItem, "loadedCachedImageData", function(item) {
          item.removeSubscriber(item, "loadedCachedImageData");

          ok(tabItem.isShowingCachedData(), 'tabItem shows cached data');

          testChangeUrlAfterReconnect();
        });

        cw.TabItems.resumeReconnecting();
      });
    });
  }

  let testChangeUrlAfterReconnect = function () {
    tab.linkedBrowser.loadURI('http://mochi.test:8888/browser/');

    whenTabAttrModified(tab, function () {
      cw.TabItems._update(tab);

      let tabItem = tab._tabViewTabItem;
      let currentLabel = tabItem.$tabTitle.text();

      is(currentLabel, 'mochitest index /browser/', 'tab label is up-to-date');
      testReconnectWithNewUrl();
    });
  }

  let testReconnectWithNewUrl = function () {
    let tabItem = tab._tabViewTabItem;
    let data = tabItem.getStorageData(true);
    gBrowser.removeTab(tab);

    cw.TabItems.pauseReconnecting();
    tab = gBrowser.loadOneTab('http://mochi.test:8888/', {inBackground: true});
    cw.Storage.saveTab(tab, data);

    whenTabAttrModified(tab, function () {
      tabItem = tab._tabViewTabItem;
      cw.TabItems.resumeReconnecting();
      ok(!tabItem.isShowingCachedData(), 'tabItem does not show cached data');

      gBrowser.removeTab(tab);
      hideTabView(finish);
    });
  }

  waitForExplicitFinish();

  showTabView(function () {
    cw = TabView.getContentWindow();
    testReconnectWithSameUrl();
  });
}


function whenTabAttrModified(tab, callback) {
  let onModified = function (event) {
    tab.removeEventListener(event.type, onModified, false);
    
    
    
    executeSoon(callback);
  }

  tab.addEventListener("TabAttrModified", onModified, false);
}
