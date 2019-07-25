





































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
  
  
  let appXulTab = gBrowser.loadOneTab("about:blank");
  gBrowser.pinTab(appXulTab);
  is(gBrowser.tabs.length, 2, "we now have two tabs");
  
  
  let box = new contentWindow.Rect(20, 20, 180, 180);
  let groupItem = new contentWindow.GroupItem([], { bounds: box });
  is(contentWindow.GroupItems.groupItems.length, 2, "we now have two groups");
  
  
  let onTabViewHidden = function() {
    window.removeEventListener("tabviewhidden", onTabViewHidden, false);
    ok(!TabView.isVisible(), "Tab View is hidden because we clicked on the app tab");
    
    
    gBrowser.unpinTab(appXulTab);
    gBrowser.removeTab(appXulTab);
    is(gBrowser.tabs.length, 1, "we finish with one tab");
  
    groupItem.close();
    is(contentWindow.GroupItems.groupItems.length, 1, "we finish with one group");
    
    ok(!TabView.isVisible(), "Tab View is not visible");
    
    finish();
  };

  window.addEventListener("tabviewhidden", onTabViewHidden, false);

  let appTabButtons = groupItem.$appTabTray[0].getElementsByTagName("img");
  ok(appTabButtons.length == 1, "there is one app tab button");
  EventUtils.sendMouseEvent({ type: "click" }, appTabButtons[0], contentWindow);
}
