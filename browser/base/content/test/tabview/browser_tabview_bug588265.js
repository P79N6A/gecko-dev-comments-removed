




































function test() {
  waitForExplicitFinish();

  window.addEventListener("tabviewshown", setup, false);
  TabView.toggle();
}

function setup() {
  window.removeEventListener("tabviewshown", setup, false);

  let contentWindow = document.getElementById("tab-view").contentWindow;
  is(contentWindow.GroupItems.groupItems.length, 1, "Has only one group");

  let groupItemOne = contentWindow.GroupItems.groupItems[0];
  
  createNewTabItemInGroupItem(groupItemOne, contentWindow, function() { 
    is(groupItemOne.getChildren().length, 2, "Group one has 2 tab items");

    
    let groupItemTwo = createEmptyGroupItem(contentWindow, 250, 250, 40, true);
    createNewTabItemInGroupItem(groupItemTwo, contentWindow, function() {
      
      testGroups(groupItemOne, groupItemTwo, contentWindow);
    });
  });
}

function createNewTabItemInGroupItem(groupItem, contentWindow, callback) {
  
  let newTabButton = groupItem.container.getElementsByClassName("newTabButton");
  ok(newTabButton[0], "New tab button exists");

  let onTabViewHidden = function() {
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);

    ok(!TabView.isVisible(), "Tab View is hidden because we just opened a tab");
    TabView.toggle();
  };
  let onTabViewShown = function() {
    window.removeEventListener("tabviewshown", onTabViewShown, false);

    ok(TabView.isVisible(), "Tab View is visible");
    callback();
  };
  window.addEventListener("tabviewhidden", onTabViewHidden, false);
  window.addEventListener("tabviewshown", onTabViewShown, false);
  EventUtils.sendMouseEvent({ type: "click" }, newTabButton[0], contentWindow);
}

function testGroups(groupItemOne, groupItemTwo, contentWindow) {
  
  is(contentWindow.GroupItems.getActiveGroupItem(), groupItemTwo, 
     "The group two is the active group");
  is(contentWindow.UI.getActiveTab(), groupItemTwo.getChild(0), 
     "The first tab item in group two is active");
  
  let tabItem = groupItemOne.getChild(1);
  tabItem.addSubscriber(tabItem, "tabRemoved", function() {
    tabItem.removeSubscriber(tabItem, "tabRemoved");

    is(groupItemOne.getChildren().length, 1,
      "The num of childen in group one is 1");

    
    is(contentWindow.GroupItems.getActiveGroupItem(), groupItemOne, 
       "The group one is the active group");
    is(contentWindow.UI.getActiveTab(), groupItemOne.getChild(0), 
       "The first tab item in group one is active");

    let onTabViewHidden = function() {
      window.removeEventListener("tabviewhidden", onTabViewHidden, false);
      is(groupItemOne.getChildren().length, 2, 
         "The num of childen in group one is 2");

      
      groupItemTwo.addSubscriber(groupItemTwo, "close", function() {
        groupItemTwo.removeSubscriber(groupItemTwo, "close");

        gBrowser.removeTab(groupItemOne.getChild(1).tab);
        is(contentWindow.GroupItems.groupItems.length, 1, "Has only one group");
        is(groupItemOne.getChildren().length, 1, 
           "The num of childen in group one is 1");
        is(gBrowser.tabs.length, 1, "Has only one tab");

        finish();
      });
      gBrowser.removeTab(groupItemTwo.getChild(0).tab);
    }
    window.addEventListener("tabviewhidden", onTabViewHidden, false);
    EventUtils.synthesizeKey("t", { accelKey: true });
  });
  
  tabItem.close();
}
