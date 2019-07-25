


let origTab = gBrowser.visibleTabs[0];
let contentWindow;

function test() {
  waitForExplicitFinish();

  test1();
}


function test1() {
  registerCleanupFunction(function () TabView.hide());

  showTabView(function() {
    ok(origTab._tabViewTabItem.parent, "The original tab belongs to a group");

    contentWindow = TabView.getContentWindow();
    contentWindow.UI.setActive(origTab._tabViewTabItem);

    testCreateTabAndThen(test2);
  });
}


function test2() {
  showTabView(function() {
    contentWindow.UI.setActive(null, { onlyRemoveActiveTab: true });

    testCreateTabAndThen(test3);
  });
}


function test3() {
  showTabView(function() {
    let groupItem = origTab._tabViewTabItem.parent;
    let tabItems = groupItem.getChildren();
    is(tabItems.length, 3, "There are 3 tab items in the group");

    let lastTabItem = tabItems[tabItems.length - 1];
    groupItem.remove(lastTabItem);

    let orphanedTabs = contentWindow.GroupItems.getOrphanedTabs();
    is(orphanedTabs.length, 1, "There should be 1 orphan tab");
    is(orphanedTabs[0], lastTabItem, "The tab item is the same as the orphan tab");

    contentWindow.UI.setActive(lastTabItem);

    testCreateTabAndThen(function() {
      hideTabView(finish);
    });
  });
}

function testCreateTabAndThen(callback) {
  ok(TabView.isVisible(), "Tab View is visible");

  
  let onTabOpen = function(event) {
    gBrowser.tabContainer.removeEventListener("TabOpen", onTabOpen, false);

    
    
    executeSoon(function() {
      let tab = event.target;
      tabItem = tab._tabViewTabItem;
      ok(tabItem, "Tab item is available after tab open");

      registerCleanupFunction(function () gBrowser.removeTab(tab))

      tabItem.addSubscriber(tabItem, "zoomedIn", function() {
        tabItem.removeSubscriber(tabItem, "zoomedIn");

        is(gBrowser.selectedTab, tab,
          "The selected tab is the same as the newly opened tab");
        executeSoon(callback);
      });
    });
  }
  gBrowser.tabContainer.addEventListener("TabOpen", onTabOpen, false);
  
  document.getElementById("menu_newNavigatorTab").doCommand();
}
