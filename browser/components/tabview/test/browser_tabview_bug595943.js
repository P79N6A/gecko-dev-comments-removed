


function test() {
  waitForExplicitFinish();

  
  window.addEventListener("tabviewshown", onTabViewWindowLoaded, false);
  TabView.toggle();
}

function onTabViewWindowLoaded() {
  window.removeEventListener("tabviewshown", onTabViewWindowLoaded, false);
  ok(TabView.isVisible(), "Tab View is visible");

  let contentWindow = document.getElementById("tab-view").contentWindow;

  
  is(contentWindow.GroupItems.groupItems.length, 1, "we start with one group (the default)"); 
  is(gBrowser.tabs.length, 1, "we start with one tab");

  let originalTab = gBrowser.tabs[0];
  ok(contentWindow.GroupItems.groupItems[0]._children[0].tab == originalTab, 
    "the original tab is in the original group");
      
  
  let box = new contentWindow.Rect(20, 20, 180, 180);
  let groupItem = new contentWindow.GroupItem([], { bounds: box });
  is(contentWindow.GroupItems.groupItems.length, 2, "we now have two groups");
  contentWindow.UI.setActive(groupItem);
  
  
  let normalXulTab = gBrowser.loadOneTab("about:blank");
  is(gBrowser.tabs.length, 2, "we now have two tabs");
  is(groupItem._children.length, 1, "the new tab was added to the group");
  
  
  let appXulTab = gBrowser.loadOneTab("about:blank");
  is(gBrowser.tabs.length, 3, "we now have three tabs");
  gBrowser.pinTab(appXulTab);
  is(groupItem._children.length, 1, "the app tab is not in the group");
  
  
  
  function onTabViewHidden() {
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);
    ok(!TabView.isVisible(), "Tab View is hidden because we clicked on the app tab");
  
    
    gBrowser.removeTab(normalXulTab);  
  
    
    ok(!TabView.isVisible(), "Tab View remains hidden");
  
    
    gBrowser.selectedTab = originalTab;
    
    gBrowser.unpinTab(appXulTab);
    gBrowser.removeTab(appXulTab);

    ok(groupItem.closeIfEmpty(), "the second group was empty");

    
    is(gBrowser.tabs.length, 1, "we finish with one tab");
    is(contentWindow.GroupItems.groupItems.length, 1,
       "we finish with one group");
    ok(!TabView.isVisible(), "we finish with Tab View hidden");
      
    finish();
  }

  window.addEventListener("tabviewhidden", onTabViewHidden, false);
  EventUtils.sendMouseEvent({ type: "mousedown" }, normalXulTab._tabViewTabItem.container, contentWindow);
  EventUtils.sendMouseEvent({ type: "mouseup" }, normalXulTab._tabViewTabItem.container, contentWindow);
}
