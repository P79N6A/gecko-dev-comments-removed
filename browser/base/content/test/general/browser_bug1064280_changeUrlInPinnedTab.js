


"use strict";

add_task(function(){
  

  let TEST_LINK_INITIAL = "about:";
  let TEST_LINK_CHANGED = "about:support";

  let appTab = gBrowser.addTab(TEST_LINK_INITIAL);
  gBrowser.pinTab(appTab);
  is(appTab.pinned, true, "Tab was successfully pinned");

  let initialTabsNo = gBrowser.tabs.length;

  let goButton = document.getElementById("urlbar-go-button");
  gBrowser.selectedTab = appTab;
  gURLBar.focus();
  gURLBar.value = TEST_LINK_CHANGED;

  let promisePageload = promiseTabLoadEvent(appTab);
  goButton.click();
  yield promisePageload;

  is(appTab.linkedBrowser.currentURI.spec, TEST_LINK_CHANGED,
     "New page loaded in the app tab");
  is(gBrowser.tabs.length, initialTabsNo, "No additional tabs were opened");
});

registerCleanupFunction(function () {
  gBrowser.removeTab(gBrowser.selectedTab);
});
