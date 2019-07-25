


function test() {
  waitForExplicitFinish();

  showTabView(onTabViewWindowLoaded);
}

let originalGroupItem = null;
let originalTab = null;

function onTabViewWindowLoaded() {
  ok(TabView.isVisible(), "Tab View is visible");

  let contentWindow = TabView.getContentWindow();

  is(contentWindow.GroupItems.groupItems.length, 1, "There is one group item on startup");
  originalGroupItem = contentWindow.GroupItems.groupItems[0];
  is(originalGroupItem.getChildren().length, 1, "There should be one Tab Item in that group.");
  contentWindow.UI.setActive(originalGroupItem);

  [originalTab] = gBrowser.visibleTabs;

  testEmptyGroupItem(contentWindow);
}

function testEmptyGroupItem(contentWindow) {
  let groupItemCount = contentWindow.GroupItems.groupItems.length;
  
  
  let emptyGroupItem = createEmptyGroupItem(contentWindow, 300, 300, 100);
  ok(emptyGroupItem.isEmpty(), "This group is empty");

  is(contentWindow.GroupItems.groupItems.length, ++groupItemCount,
     "The number of groups is increased by 1");

  emptyGroupItem.addSubscriber("close", function onClose() {
    emptyGroupItem.removeSubscriber("close", onClose);

    
    is(contentWindow.GroupItems.groupItems.length, --groupItemCount,
       "The number of groups is decreased by 1");

    testGroupItemWithTabItem(contentWindow);
  });

  let closeButton = emptyGroupItem.container.getElementsByClassName("close");
  ok(closeButton[0], "Group close button exists");

  
  EventUtils.synthesizeMouse(closeButton[0], 1, 1, {}, contentWindow);
}

function testGroupItemWithTabItem(contentWindow) {
  let groupItem = createEmptyGroupItem(contentWindow, 300, 300, 200);
  let tabItemCount = 0;

  let onTabViewShown = function() {
    let tabItem = groupItem.getChild(groupItem.getChildren().length - 1);
    ok(tabItem, "Tab item exists");

    let tabItemClosed = false;
    tabItem.addSubscriber("close", function onClose() {
      tabItem.removeSubscriber("close", onClose);
      tabItemClosed = true;
    });
    tabItem.addSubscriber("tabRemoved", function onTabRemoved() {
      tabItem.removeSubscriber("tabRemoved", onTabRemoved);

      ok(tabItemClosed, "The tab item is closed");
      is(groupItem.getChildren().length, --tabItemCount,
        "The number of children in new tab group is decreased by 1");

      ok(TabView.isVisible(), "Tab View is still shown");

      
      
      is(gBrowser.tabs.length, 1, "There is only one tab left");

      
      
      
      let tabItems = contentWindow.TabItems.getItems();
      ok(tabItems[0], "A tab item exists");
      contentWindow.UI.setActive(tabItems[0]);

      hideTabView(function() {
        ok(!TabView.isVisible(), "Tab View is hidden");

        closeGroupItem(groupItem, finish);
      });
    });

    
    let closeButton = tabItem.container.getElementsByClassName("close");
    ok(closeButton, "Tab item close button exists");

    EventUtils.sendMouseEvent({ type: "mousedown" }, closeButton[0], contentWindow);
    EventUtils.sendMouseEvent({ type: "mouseup" }, closeButton[0], contentWindow);
  };

  whenTabViewIsHidden(function() {
    is(groupItem.getChildren().length, ++tabItemCount,
       "The number of children in new tab group is increased by 1");

    ok(!TabView.isVisible(), "Tab View is hidden because we just opened a tab");
    showTabView(onTabViewShown);
  });
  groupItem.newTab();
}
