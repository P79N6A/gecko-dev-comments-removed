




































function test() {
  waitForExplicitFinish();

  window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
  TabView.toggle();
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);
  ok(TabView.isVisible(), "Tab View is visible");

  let contentWindow = document.getElementById("tab-view").contentWindow;
  testEmptyGroupItem(contentWindow);
}

function testEmptyGroupItem(contentWindow) {
  let groupItemCount = contentWindow.GroupItems.groupItems.length;

  
  let emptyGroupItem = createEmptyGroupItem(contentWindow, 100);
  ok(emptyGroupItem.isEmpty(), "This group is empty");

  is(contentWindow.GroupItems.groupItems.length, ++groupItemCount,
     "The number of groups is increased by 1");

  emptyGroupItem.addSubscriber(emptyGroupItem, "close", function() {
    emptyGroupItem.removeSubscriber(emptyGroupItem, "close");

    
    is(contentWindow.GroupItems.groupItems.length, --groupItemCount,
       "The number of groups is decreased by 1");

    testGroupItemWithTabItem(contentWindow);
  });

  let closeButton = emptyGroupItem.container.getElementsByClassName("close");
  ok(closeButton[0], "Group close button exists");

  
  EventUtils.synthesizeMouse(closeButton[0], 1, 1, {}, contentWindow);
}

function testGroupItemWithTabItem(contentWindow) {
  let groupItem = createEmptyGroupItem(contentWindow, 200);
  let tabItemCount = 0;

  groupItem.addSubscriber(groupItem, "tabAdded", function() {
    groupItem.removeSubscriber(groupItem, "tabAdded");
    TabView.toggle();
  });

  let onTabViewHidden = function() {
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);

    is(groupItem.getChildren().length, ++tabItemCount,
       "The number of children in new tab group is increased by 1");

    let tabItem = groupItem.getChild(groupItem.getChildren().length - 1);
    ok(tabItem, "Tab item exists");

    let tabRemoved = false;
    tabItem.addSubscriber(tabItem, "close", function() {
      tabItem.removeSubscriber(tabItem, "close");

      ok(tabRemoved, "Tab is removed");
      
      is(groupItem.getChildren().length, --tabItemCount,
        "The number of children in new tab group is decreased by 1");
      finish();
    });
    tabItem.addSubscriber(tabItem, "tabRemoved", function() {
      tabItem.removeSubscriber(tabItem, "tabRemoved");
      tabRemoved = true;
    });

    
    let closeButton = tabItem.container.getElementsByClassName("close");
    ok(closeButton, "Tab item close button exists");

    EventUtils.sendMouseEvent({ type: "mousedown" }, closeButton[0], contentWindow);
    EventUtils.sendMouseEvent({ type: "mouseup" }, closeButton[0], contentWindow);
  };

  window.addEventListener("tabviewhidden", onTabViewHidden, false);
  
  let newTabButton = groupItem.container.getElementsByClassName("newTabButton");
  ok(newTabButton[0], "New tab button exists");

  EventUtils.synthesizeMouse(newTabButton[0], 1, 1, {}, contentWindow);
}

function createEmptyGroupItem(contentWindow, padding) {
  let pageBounds = contentWindow.Items.getPageBounds();
  pageBounds.inset(padding, padding);

  let box = new contentWindow.Rect(pageBounds);
  box.width = 300;
  box.height = 300;

  let emptyGroupItem = new contentWindow.GroupItem([], { bounds: box });

  return emptyGroupItem;
}
