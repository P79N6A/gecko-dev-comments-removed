











function test() {
  let originalTab = gBrowser.selectedTab;
  let newTab = gBrowser.addTab("about:blank", {skipAnimation: true});
  is(newTab.lastAccessed, 0, "Timestamp on the new tab is 0.");
  gBrowser.selectedTab = newTab;
  let newTabAccessedDate = newTab.lastAccessed;
  ok(newTabAccessedDate > 0, "Timestamp on the selected tab is more than 0.");
  ok(newTabAccessedDate <= Date.now(), "Timestamp less than or equal current Date.");
  gBrowser.selectedTab = originalTab;
  is(newTab.lastAccessed, newTabAccessedDate, "New tab's timestamp remains the same.");
  gBrowser.removeTab(newTab);
}
