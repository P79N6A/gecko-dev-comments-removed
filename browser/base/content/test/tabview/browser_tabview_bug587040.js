




































function test() {
  waitForExplicitFinish();

  window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
  TabView.toggle();
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);

  ok(TabView.isVisible(), "Tab View is visible");

  let [originalTab] = gBrowser.visibleTabs;
  let contentWindow = document.getElementById("tab-view").contentWindow;

  
  let box = new contentWindow.Rect(20, 20, 300, 300);
  let groupItem = new contentWindow.GroupItem([], { bounds: box });

  let onTabViewShown = function() {
    window.removeEventListener("tabviewshown", onTabViewShown, false);

    is(groupItem.getChildren().length, 2, "The new group has two tab items");
    
    testSameTabItemAndClickGroup(contentWindow, groupItem);
  };

  
  let onTabViewHidden = function() {
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);

    ok(!TabView.isVisible(), "Tab View is hidden because we just opened a tab");

    let anotherTab = gBrowser.addTab("http://mochi.test:8888/");
    let browser = gBrowser.getBrowserForTab(anotherTab);
    let onLoad = function() {
      browser.removeEventListener("load", onLoad, true);
    
      window.addEventListener("tabviewshown", onTabViewShown, false);
      TabView.toggle();
    }
    browser.addEventListener("load", onLoad, true);
  };
  window.addEventListener("tabviewhidden", onTabViewHidden, false);

  
  let newTabButton = groupItem.container.getElementsByClassName("newTabButton");
  ok(newTabButton[0], "New tab button exists");

  EventUtils.sendMouseEvent({ type: "click" }, newTabButton[0], contentWindow);
}

function testSameTabItemAndClickGroup(contentWindow, groupItem, originalTab) {
  let activeTabItem = groupItem.getActiveTab();
  ok(activeTabItem, "Has active item");

  let onTabViewHidden = function() {
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);

    is(activeTabItem.tab, gBrowser.selectedTab, "The correct tab is selected");
    TabView.toggle();
  };

  let onTabViewShown = function() {
    window.removeEventListener("tabviewshown", onTabViewShown, false);

    testDifferentTabItemAndClickGroup(contentWindow, groupItem, originalTab);
  };
  window.addEventListener("tabviewhidden", onTabViewHidden, false);
  window.addEventListener("tabviewshown", onTabViewShown, false);
  
  let groupElement = groupItem.container;
  EventUtils.sendMouseEvent({ type: "mousedown" }, groupElement, contentWindow);
  EventUtils.sendMouseEvent({ type: "mouseup" }, groupElement, contentWindow);
}

function testDifferentTabItemAndClickGroup(contentWindow, groupItem, originalTab) {
  let activeTabItem = groupItem.getActiveTab();
  ok(activeTabItem, "Has old active item");

  
  EventUtils.synthesizeKey("VK_TAB", {}, contentWindow);

  let newActiveTabItem = groupItem.getActiveTab();
  ok(newActiveTabItem, "Has new active item");

  isnot(newActiveTabItem, activeTabItem, "The old and new active items are different");

  let onTabViewHidden = function() {
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);

    is(newActiveTabItem.tab, gBrowser.selectedTab, "The correct tab is selected");
    
    
    groupItem.addSubscriber(groupItem, "close", function() {
      groupItem.removeSubscriber(groupItem, "close");
      finish();  
    });
    gBrowser.selectedTab = originalTab;

    groupItem.closeAll();
    
    let closeButton = groupItem.$undoContainer.find(".close");
    EventUtils.sendMouseEvent(
      { type: "click" }, closeButton[0], contentWindow);
  };
  window.addEventListener("tabviewhidden", onTabViewHidden, false);

  
  let groupElement = groupItem.container;
  EventUtils.sendMouseEvent({ type: "mousedown" }, groupElement, contentWindow);
  EventUtils.sendMouseEvent({ type: "mouseup" }, groupElement, contentWindow);
}
