


let contentWindow;
let appTab;
let originalTab;
let exitButton;


function test() {
  waitForExplicitFinish();

  window.addEventListener("tabviewshown", onTabViewLoadedAndShown, false);
  TabView.toggle();
}


function onTabViewLoadedAndShown() {
  window.removeEventListener("tabviewshown", onTabViewLoadedAndShown, false);
  ok(TabView.isVisible(), "Tab View is visible");

  contentWindow = document.getElementById("tab-view").contentWindow;

  
  is(contentWindow.GroupItems.groupItems.length, 1,
      "we start with one group (the default)");
  is(gBrowser.tabs.length, 1, "we start with one tab");
  originalTab = gBrowser.tabs[0];
  ok(!originalTab.pinned, "the original tab is not an app tab");

  
  appTab = gBrowser.loadOneTab("about:blank");
  is(gBrowser.tabs.length, 2, "we now have two tabs");
  gBrowser.pinTab(appTab);

  
  ok(gBrowser.selectedTab == originalTab, "the normal tab is selected");

  
  exitButton = contentWindow.document.getElementById("exit-button");
  ok(exitButton, "Exit button exists");

  window.addEventListener("tabviewhidden", onTabViewHiddenForNormalTab, false);
  EventUtils.sendMouseEvent({ type: "click" }, exitButton, contentWindow);
}


function onTabViewHiddenForNormalTab() {
  window.removeEventListener("tabviewhidden", onTabViewHiddenForNormalTab, false);
  ok(!TabView.isVisible(), "Tab View is not visible");

  
  ok(gBrowser.selectedTab == originalTab, "the normal tab is still selected");

  
  gBrowser.selectedTab = appTab;
  ok(gBrowser.selectedTab == appTab, "the app tab is now selected");

  
  window.addEventListener("tabviewshown", onTabViewShown, false);
  TabView.toggle();
}


function onTabViewShown() {
  window.removeEventListener("tabviewshown", onTabViewShown, false);
  ok(TabView.isVisible(), "Tab View is visible");

  
  window.addEventListener("tabviewhidden", onTabViewHiddenForAppTab, false);
  EventUtils.sendMouseEvent({ type: "click" }, exitButton, contentWindow);
}


function onTabViewHiddenForAppTab() {
  window.removeEventListener("tabviewhidden", onTabViewHiddenForAppTab, false);
  ok(!TabView.isVisible(), "Tab View is not visible");

  
  ok(gBrowser.selectedTab == appTab, "the app tab is still selected");

  
  gBrowser.selectedTab = originalTab; 
  gBrowser.removeTab(appTab);

  is(gBrowser.tabs.length, 1, "we finish with one tab");
  ok(gBrowser.selectedTab == originalTab,
      "we finish with the normal tab selected");
  ok(!TabView.isVisible(), "we finish with Tab View not visible");

  finish();
}
