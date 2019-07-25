


function test() {
  waitForExplicitFinish();

  newWindowWithTabView(onTabViewShown);
}

function onTabViewShown(win) {
  let contentWindow = win.document.getElementById("tab-view").contentWindow;
  is(contentWindow.GroupItems.groupItems.length, 1, "Has only one group");

  let originalTab = win.gBrowser.selectedTab;
  let originalGroup = contentWindow.GroupItems.groupItems[0];
  let newTab = win.gBrowser.loadOneTab("about:blank", {inBackground: true});
  
  is(originalGroup.getChildren().length, 2, "The original group now has two tabs");
  
  
  let box = new contentWindow.Rect(300,300,150,150);
  let newGroup = new contentWindow.GroupItem([], {bounds: box, immediately: true});
  newGroup.add(newTab._tabViewTabItem, {immediately: true});

  
  contentWindow.GroupItems.setActiveGroupItem(originalGroup);
  is(contentWindow.GroupItems.getActiveGroupItem(), originalGroup,
     "The original group is active");
  is(contentWindow.UI.getActiveTab(), originalTab._tabViewTabItem,
     "The original tab is active");
  
  function checkActive(callback, time) {
    is(contentWindow.GroupItems.getActiveGroupItem(), newGroup,
       "The new group is active");
    is(contentWindow.UI.getActiveTab(), newTab._tabViewTabItem,
       "The new tab is active");
    if (time)
      setTimeout(callback, time);
    else
      callback();
  }

  
  
  
  
  EventUtils.sendMouseEvent({ type: "mousedown" },
                            newTab._tabViewTabItem.container, contentWindow);
  EventUtils.sendMouseEvent({ type: "mouseup" },
                            newTab._tabViewTabItem.container, contentWindow);
  setTimeout(function() {
    checkActive(function() {
      checkActive(function() {
        win.close();
        finish();
      });
    }, 490);
  }, 10)
}
