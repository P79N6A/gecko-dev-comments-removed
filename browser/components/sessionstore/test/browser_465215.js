"use strict";

let uniqueName = "bug 465215";
let uniqueValue1 = "as good as unique: " + Date.now();
let uniqueValue2 = "as good as unique: " + Math.random();

add_task(function* () {
  
  let tab1 = gBrowser.addTab("about:blank");
  yield promiseBrowserLoaded(tab1.linkedBrowser);
  ss.setTabValue(tab1, uniqueName, uniqueValue1);

  
  let tab2 = ss.duplicateTab(window, tab1);
  yield promiseTabRestored(tab2);
  is(ss.getTabValue(tab2, uniqueName), uniqueValue1, "tab value was duplicated");

  ss.setTabValue(tab2, uniqueName, uniqueValue2);
  isnot(ss.getTabValue(tab1, uniqueName), uniqueValue2, "tab values aren't sync'd");

  
  yield promiseTabState(tab1, {entries: []});
  is(ss.getTabValue(tab1, uniqueName), "", "tab value was cleared");

  
  gBrowser.removeTab(tab2);
  gBrowser.removeTab(tab1);
});
