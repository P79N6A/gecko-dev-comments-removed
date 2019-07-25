





































function test() {
  waitForExplicitFinish();

  window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
  TabView.toggle();
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);

  ok(TabView.isVisible(), "Tab View is visible");

  let contentWindow = document.getElementById("tab-view").contentWindow;
  let [originalTab] = gBrowser.visibleTabs;

  
  let padding = 10;
  let pageBounds = contentWindow.Items.getPageBounds();
  pageBounds.inset(padding, padding);

  let boxOne = new contentWindow.Rect(20, 20, 300, 300);
  let groupOne = new contentWindow.GroupItem([], { bounds: boxOne });
  ok(groupOne.isEmpty(), "This group is empty");

  let boxTwo = new contentWindow.Rect(20, 400, 300, 300);
  let groupTwo = new contentWindow.GroupItem([], { bounds: boxTwo });

  groupOne.addSubscriber(groupOne, "childAdded", function() {
    groupOne.removeSubscriber(groupOne, "childAdded");
    groupTwo.newTab();
  });

  let count = 0;
  let onTabViewShown = function() {
    if (count == 2) {
      window.removeEventListener("tabviewshown", onTabViewShown, false);
      addTest(contentWindow, groupOne.id, groupTwo.id, originalTab);
    }
  };
  let onTabViewHidden = function() {
    TabView.toggle();
    if (++count == 2)
      window.removeEventListener("tabviewhidden", onTabViewHidden, false);
  };
  window.addEventListener("tabviewshown", onTabViewShown, false);
  window.addEventListener("tabviewhidden", onTabViewHidden, false);

  
  groupOne.newTab();
}

function addTest(contentWindow, groupOneId, groupTwoId, originalTab) {
  let groupOne = contentWindow.GroupItems.groupItem(groupOneId);
  let groupTwo = contentWindow.GroupItems.groupItem(groupTwoId);
  let groupOneTabItemCount = groupOne.getChildren().length;
  let groupTwoTabItemCount = groupTwo.getChildren().length;
  is(groupOneTabItemCount, 1, "GroupItem one has one tab");
  is(groupTwoTabItemCount, 1, "GroupItem two has one tab as well");

  let tabItem = groupOne.getChild(0);
  ok(tabItem, "The tab item exists");

  
  let groupTwoRect = groupTwo.getBounds();
  let groupTwoRectCenter = groupTwoRect.center();
  let tabItemRect = tabItem.getBounds();
  let tabItemRectCenter = tabItemRect.center();
  let offsetX =
    Math.round(groupTwoRectCenter.x - tabItemRectCenter.x);
  let offsetY =
    Math.round(groupTwoRectCenter.y - tabItemRectCenter.y);

  function endGame() {
    groupTwo.removeSubscriber(groupTwo, "childAdded");

    is(groupOne.getChildren().length, --groupOneTabItemCount,
       "The number of children in group one is decreased by 1");
    is(groupTwo.getChildren().length, ++groupTwoTabItemCount,
       "The number of children in group two is increased by 1");
  
    let onTabViewHidden = function() {
      window.removeEventListener("tabviewhidden", onTabViewHidden, false);
       groupTwo.closeAll();
    };
    groupTwo.addSubscriber(groupTwo, "close", function() {
      groupTwo.removeSubscriber(groupTwo, "close");
      finish();  
    });
    window.addEventListener("tabviewhidden", onTabViewHidden, false);
    gBrowser.selectedTab = originalTab;
  }
  groupTwo.addSubscriber(groupTwo, "childAdded", endGame);
  
  simulateDragDrop(tabItem.container, offsetX, offsetY, contentWindow);
}

function simulateDragDrop(tabItem, offsetX, offsetY, contentWindow) {
  
  let dataTransfer;

  EventUtils.synthesizeMouse(
    tabItem, 1, 1, { type: "mousedown" }, contentWindow);
  event = contentWindow.document.createEvent("DragEvents");
  event.initDragEvent(
    "dragenter", true, true, contentWindow, 0, 0, 0, 0, 0,
    false, false, false, false, 1, null, dataTransfer);
  tabItem.dispatchEvent(event);

  
  if (offsetX || offsetY) {
    let Ci = Components.interfaces;
    let utils = contentWindow.QueryInterface(Ci.nsIInterfaceRequestor).
                              getInterface(Ci.nsIDOMWindowUtils);
    let rect = tabItem.getBoundingClientRect();
    for (let i = 1; i <= 5; i++) {
      let left = rect.left + Math.round(i * offsetX / 5);
      let top = rect.top + Math.round(i * offsetY / 5);
      utils.sendMouseEvent("mousemove", left, top, 0, 1, 0);
    }
    event = contentWindow.document.createEvent("DragEvents");
    event.initDragEvent(
      "dragover", true, true, contentWindow, 0, 0, 0, 0, 0,
      false, false, false, false, 0, null, dataTransfer);
    tabItem.dispatchEvent(event);
  }
  
  
  EventUtils.synthesizeMouse(tabItem, 0, 0, { type: "mouseup" }, contentWindow);
  event = contentWindow.document.createEvent("DragEvents");
  event.initDragEvent(
    "drop", true, true, contentWindow, 0, 0, 0, 0, 0,
    false, false, false, false, 0, null, dataTransfer);
  tabItem.dispatchEvent(event);
}
