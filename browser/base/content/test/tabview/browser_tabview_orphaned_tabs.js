




































let tabOne;
let newWin;

function test() {
  waitForExplicitFinish();

  newWin = 
    window.openDialog(getBrowserURL(), "_blank", "all,dialog=no", "about:blank");

  let onLoad = function() {
    newWin.removeEventListener("load", onLoad, false);

    tabOne = newWin.gBrowser.addTab();

    newWin.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
    newWin.TabView.toggle();
  }
  newWin.addEventListener("load", onLoad, false);
}

function onTabViewWindowLoaded() {
  newWin.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);

  ok(newWin.TabView.isVisible(), "Tab View is visible");

  let contentWindow = newWin.document.getElementById("tab-view").contentWindow;

  
  ok(tabOne.tabItem.parent, "Tab one belongs to a group");
  is(contentWindow.GroupItems.getOrphanedTabs().length, 0, "No orphaned tabs");

  
  let groupItem = createEmptyGroupItem(contentWindow, 300, 300, 200);

  let onTabViewHidden = function() {
    newWin.removeEventListener("tabviewhidden", onTabViewHidden, false);

    
    is(groupItem.getChildren().length, 1, "The group item has an item");
    is(contentWindow.GroupItems.getOrphanedTabs().length, 0, "No orphaned tabs");
    
    let checkAndFinish = function() {
      
      let tabData = contentWindow.Storage.getTabData(tabItem.tab);
      ok(tabData && contentWindow.TabItems.storageSanity(tabData) && tabData.groupID, 
         "Tab two has stored group data");

      
      newWin.gBrowser.removeTab(tabOne);
      newWin.gBrowser.removeTab(tabItem.tab);
      whenWindowObservesOnce(newWin, "domwindowclosed", function() {
        finish();
      });
      newWin.close();
    };
    let tabItem = groupItem.getChild(0);
    
    if (tabItem.reconnected) {
      checkAndFinish();
    } else {
      tabItem.addSubscriber(tabItem, "reconnected", function() {
        tabItem.removeSubscriber(tabItem, "reconnected");
        checkAndFinish();
      });
    }
  };
  newWin.addEventListener("tabviewhidden", onTabViewHidden, false);

  
  let newTabButton = groupItem.container.getElementsByClassName("newTabButton");
  ok(newTabButton[0], "New tab button exists");

  EventUtils.sendMouseEvent({ type: "click" }, newTabButton[0], contentWindow);
}

function whenWindowObservesOnce(win, topic, callback) {
  let windowWatcher = 
    Cc["@mozilla.org/embedcomp/window-watcher;1"].getService(Ci.nsIWindowWatcher);
  function windowObserver(subject, topicName, aData) {
    if (win == subject.QueryInterface(Ci.nsIDOMWindow) && topic == topicName) {
      windowWatcher.unregisterNotification(windowObserver);
      callback();
    }
  }
  windowWatcher.registerNotification(windowObserver);
}
