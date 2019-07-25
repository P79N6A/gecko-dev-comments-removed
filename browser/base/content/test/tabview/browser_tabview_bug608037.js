


let tabOne;
let tabTwo;

function test() {
  waitForExplicitFinish();

  tabOne = gBrowser.addTab("http://mochi.test:8888/");
  tabTwo = gBrowser.addTab("http://mochi.test:8888/browser/browser/base/content/test/tabview/dummy_page.html");

  afterAllTabsLoaded(function () {
    
    
    gBrowser.selectedTab = tabOne;
    showTabView(onTabViewShown);
  });
}

function onTabViewShown() {
  let contentWindow = TabView.getContentWindow();
  let groupItems = contentWindow.GroupItems.groupItems;
  is(groupItems.length, 1, "There is only one group");
  is(groupItems[0].getChildren().length, 3, "The group has three tab items");

  gBrowser.removeTab(tabTwo);
  ok(TabView.isVisible(), "Tab View is still visible after removing a tab");
  is(groupItems[0].getChildren().length, 2, "The group has two tab items");

  restoreTab(function (tabTwo) {
    ok(TabView.isVisible(), "Tab View is still visible after restoring a tab");
    is(groupItems[0].getChildren().length, 3, "The group still has three tab items");

    
    hideTabView(function () {
      gBrowser.removeTab(tabOne);
      gBrowser.removeTab(tabTwo);
      finish();
    });
  });
}
