


function test() {
  waitForExplicitFinish();

  let AllTabs;
  let newTab = gBrowser.addTab();

  
  let pinned = function (event) {
    let tab = event.target;

    is(tab, newTab, "The tabs are the same after the tab is pinned");
    ok(tab.pinned, "The tab gets pinned");

    gBrowser.unpinTab(tab);
  };

  
  let unpinned = function (event) {
    let tab = event.target;

    AllTabs.unregister("pinned", pinned);
    AllTabs.unregister("unpinned", unpinned);

    is(tab, newTab, "The tabs are the same after the tab is unpinned");
    ok(!tab.pinned, "The tab gets unpinned");

    
    gBrowser.removeTab(tab);
    hideTabView(finish);
  };

  showTabView(function () {
    AllTabs = TabView.getContentWindow().AllTabs;

    AllTabs.register("pinned", pinned);
    AllTabs.register("unpinned", unpinned);

    ok(!newTab.pinned, "The tab is not pinned");
    gBrowser.pinTab(newTab);
  });
}

