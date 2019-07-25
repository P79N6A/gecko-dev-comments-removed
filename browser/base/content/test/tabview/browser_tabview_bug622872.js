


function test() {
  waitForExplicitFinish();
  newWindowWithTabView(part1);
}





function part1(win) {
  ok(win.TabView.isVisible(), "Tab View is visible");

  let contentWindow = win.document.getElementById("tab-view").contentWindow;
  is(win.gBrowser.tabs.length, 1, "In the beginning, there was one tab.");
  let [originalTab] = win.gBrowser.visibleTabs;
  let originalGroup = contentWindow.GroupItems.getActiveGroupItem();
  ok(originalGroup.getChildren().some(function(child) {
    return child == originalTab._tabViewTabItem;
  }),"The current active group is the one with the original tab in it.");

  
  let newTab = win.gBrowser.loadOneTab("about:mozilla", {inBackground: true});

  let newTabItem = newTab._tabViewTabItem;
  ok(originalGroup.getChildren().some(function(child) child == newTabItem),"The new tab was made in the current group");
  contentWindow.GroupItems.getActiveGroupItem().remove(newTabItem);
  ok(!originalGroup.getChildren().some(function(child) child == newTabItem),"The new tab was orphaned");
  newTabItem.pushAway();
  
  contentWindow.UI.setActive(newTabItem);

  
  let part2 = function part2() {
    win.removeEventListener("tabviewhidden", part2, false);

    is(win.gBrowser.selectedTab, newTab, "We zoomed into that new tab.");
    ok(!win.TabView.isVisible(), "Tab View is hidden, because we're looking at the new tab");

    newTab.addEventListener("TabClose", function() {
      newTab.removeEventListener("TabClose", arguments.callee, false);

      win.addEventListener("tabviewshown", part3, false);
      executeSoon(function() { win.TabView.toggle(); });
    }, false);
    win.gBrowser.removeTab(newTab);
  }

  let secondNewTab;
  
  
  let part3 = function part3() {
    win.removeEventListener("tabviewshown", part3, false);

    ok(win.TabView.isVisible(), "Tab View is visible.");

    is(win.gBrowser.tabs.length, 1, "Only one tab again.");
    is(win.gBrowser.tabs[0], originalTab, "That one tab is the original tab.");

    let groupItems = contentWindow.GroupItems.groupItems;
    is(groupItems.length, 1, "Only one group");

    ok(!contentWindow.UI.getActiveOrphanTab(), "There is no active orphan tab.");
    ok(win.TabView.isVisible(), "Tab View is visible.");
  
    whenTabViewIsHidden(part4, win);
    win.document.getElementById("cmd_newNavigatorTab").doCommand();
  }

  
  let part4 = function part4() {
    ok(!win.TabView.isVisible(), "Tab View is hidden");

    is(win.gBrowser.tabs.length, 2, "There are two tabs total now.");
    is(win.gBrowser.visibleTabs.length, 2, "We're looking at both of them.");

    let foundOriginalTab = false;
    
    for (let i = 0; i < win.gBrowser.tabs.length; i++) {
      let tab = win.gBrowser.tabs[i];
      if (tab === originalTab)
        foundOriginalTab = true;
      else
        secondNewTab = tab;
    }
    ok(foundOriginalTab, "One of those tabs is the original tab.");
    ok(secondNewTab, "We found another tab... this is secondNewTab");

    is(win.gBrowser.selectedTab, secondNewTab, "This second new tab is what we're looking at.");

    win.addEventListener("tabviewshown", part5, false);
    win.TabView.toggle();
  }

  
  let part5 = function part5() {
    win.removeEventListener("tabviewshown", part5, false);

    is(win.gBrowser.tabs.length, 2, "There are of course still two tabs.");

    let groupItems = contentWindow.GroupItems.groupItems;
    is(groupItems.length, 1, "There is one group now");
    is(groupItems[0], originalGroup, "It's the original group.");
    is(originalGroup.getChildren().length, 2, "It has two children.");

    
    win.close();
    finish();
  }

  
  afterAllTabItemsUpdated(function() {
    win.addEventListener("tabviewhidden", part2, false);
    win.TabView.toggle();
  }, win);
}
