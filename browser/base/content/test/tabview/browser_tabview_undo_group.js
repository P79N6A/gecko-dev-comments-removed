


function test() {
  waitForExplicitFinish();

  window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
  TabView.toggle();
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);
  ok(TabView.isVisible(), "Tab View is visible");

  let contentWindow = document.getElementById("tab-view").contentWindow;

  
  let box = new contentWindow.Rect(20, 400, 300, 300);
  let groupItem = new contentWindow.GroupItem([], { bounds: box });

  
  let onTabViewHidden = function() {
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);

    ok(!TabView.isVisible(), "Tab View is hidden because we just opened a tab");
    
    TabView.toggle();
  };
  let onTabViewShown = function() {
    window.removeEventListener("tabviewshown", onTabViewShown, false);

    is(groupItem.getChildren().length, 1, "The new group has a tab item");
    
    waitForFocus(function() {
      testUndoGroup(contentWindow, groupItem);
    }, contentWindow);
  };
  window.addEventListener("tabviewhidden", onTabViewHidden, false);
  window.addEventListener("tabviewshown", onTabViewShown, false);

  
  let newTabButton = groupItem.container.getElementsByClassName("newTabButton");
  ok(newTabButton[0], "New tab button exists");

  EventUtils.sendMouseEvent({ type: "click" }, newTabButton[0], contentWindow);
}

function testUndoGroup(contentWindow, groupItem) {
  groupItem.addSubscriber(groupItem, "groupHidden", function() {
    groupItem.removeSubscriber(groupItem, "groupHidden");

    
    let theGroupItem = contentWindow.GroupItems.groupItem(groupItem.id);
    ok(theGroupItem, "The group item still exists");
    is(theGroupItem.getChildren().length, 1, 
       "The tab item in the group still exists");

    
    is(theGroupItem.container.style.display, "none", 
       "The group element is hidden");
    ok(theGroupItem.$undoContainer, "Undo container is avaliable");

    EventUtils.sendMouseEvent(
      { type: "click" }, theGroupItem.$undoContainer[0], contentWindow);
  });

  groupItem.addSubscriber(groupItem, "groupShown", function() {
    groupItem.removeSubscriber(groupItem, "groupShown");

    
    let theGroupItem = contentWindow.GroupItems.groupItem(groupItem.id);
    ok(theGroupItem, "The group item still exists");
    is(theGroupItem.getChildren().length, 1, 
       "The tab item in the group still exists");

    
    is(theGroupItem.container.style.display, "", "The group element is visible");
    ok(!theGroupItem.$undoContainer, "Undo container is not avaliable");
    
    
    testCloseUndoGroup(contentWindow, groupItem);
  });

  let closeButton = groupItem.container.getElementsByClassName("close");
  ok(closeButton[0], "Group item close button exists");
  EventUtils.sendMouseEvent({ type: "click" }, closeButton[0], contentWindow);
}

function testCloseUndoGroup(contentWindow, groupItem) {
  groupItem.addSubscriber(groupItem, "groupHidden", function() {
    groupItem.removeSubscriber(groupItem, "groupHidden");

    
    let theGroupItem = contentWindow.GroupItems.groupItem(groupItem.id);
    ok(theGroupItem, "The group item still exists");
    is(theGroupItem.getChildren().length, 1, 
       "The tab item in the group still exists");

    
    is(theGroupItem.container.style.display, "none", 
       "The group element is hidden");
    ok(theGroupItem.$undoContainer, "Undo container is avaliable");

    
    let closeButton = theGroupItem.$undoContainer.find(".close");
    EventUtils.sendMouseEvent(
      { type: "click" }, closeButton[0], contentWindow);
  });

  groupItem.addSubscriber(groupItem, "close", function() {
    groupItem.removeSubscriber(groupItem, "close");

    let theGroupItem = contentWindow.GroupItems.groupItem(groupItem.id);
    ok(!theGroupItem, "The group item doesn't exists");

    let endGame = function() {
      window.removeEventListener("tabviewhidden", endGame, false);
      ok(!TabView.isVisible(), "Tab View is hidden");
      finish();
    };
    window.addEventListener("tabviewhidden", endGame, false);

    
    
    
    let tabItems = contentWindow.TabItems.getItems();
    ok(tabItems[0], "A tab item exists");
    contentWindow.UI.setActive(tabItems[0]);

    TabView.toggle();
  });

  let closeButton = groupItem.container.getElementsByClassName("close");
  ok(closeButton[0], "Group item close button exists");
  EventUtils.sendMouseEvent({ type: "click" }, closeButton[0], contentWindow);
}
