




































let tabOne;
let tabTwo;

function test() {
  waitForExplicitFinish();

  tabOne = gBrowser.addTab("http://mochi.test:8888/");
  tabTwo = gBrowser.addTab("http://mochi.test:8888/browser/browser/base/content/test/tabview/dummy_page.html");

  
  let stillToLoad = 0;
  let onLoad = function(event) {
    event.target.removeEventListener("load", onLoad, true);

    stillToLoad--;
    if (!stillToLoad) {
      
      window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
      ok(!TabView.isVisible(), "Tab View is hidden");

      
      
      gBrowser.selectedTab = tabOne;

      TabView.toggle();
    }
  }

  let newTabs = [ tabOne, tabTwo ];
  newTabs.forEach(function(tab) {
    stillToLoad++; 
    tab.linkedBrowser.addEventListener("load", onLoad, true);
  });
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);

  let contentWindow = document.getElementById("tab-view").contentWindow;
  let groupItems = contentWindow.GroupItems.groupItems;
  is(groupItems.length, 1, "There is only one group");
  is(groupItems[0].getChildren().length, 3, "The group has three tab items");

  gBrowser.removeTab(tabTwo);
  ok(TabView.isVisible(), "Tab View is still visible after removing a tab");
  is(groupItems[0].getChildren().length, 2, "The group has two tab items");

  tabTwo = undoCloseTab(0);
  ok(TabView.isVisible(), "Tab View is still visible after restoring a tab");
  is(groupItems[0].getChildren().length, 3, "The group has three tab items");

  
  let endGame = function() {
    window.removeEventListener("tabviewhidden", endGame, false);

    gBrowser.removeTab(tabOne);
    gBrowser.removeTab(tabTwo);
    finish();
  }
  window.addEventListener("tabviewhidden", endGame, false);
  TabView.toggle();
}
