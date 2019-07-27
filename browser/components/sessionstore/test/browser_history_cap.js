


"use strict";









add_task(function *test_history_cap() {
  const baseURL = "http://example.com/browser_history_cap#"
  const maxEntries  = 9; 
  const middleEntry = 4; 

  const maxBack1 = 2; 
  const maxFwd1 = 3;  

  const maxBack2 = 5; 
  const maxFwd2 = 5;  

  
  gPrefService.setIntPref("browser.sessionhistory.max_entries", maxEntries);
  gPrefService.setIntPref("browser.sessionstore.max_serialize_back", maxBack1);
  gPrefService.setIntPref("browser.sessionstore.max_serialize_forward", maxFwd1);

  
  registerCleanupFunction(() => {
    gPrefService.clearUserPref("browser.sessionhistory.max_entries");
    gPrefService.clearUserPref("browser.sessionstore.max_serialize_back");
    gPrefService.clearUserPref("browser.sessionstore.max_serialize_forward");
  });

  let tab = gBrowser.addTab();
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  
  let tabState = {entries: [], index: middleEntry + 1};
  for (let i = 0; i < maxEntries; i++) {
    tabState.entries.push({url: baseURL + i});
  }

  info("Testing situation where only a subset of session history entries should be restored.");

  ss.setTabState(tab, JSON.stringify(tabState));
  yield promiseTabRestored(tab);
  TabState.flush(tab.linkedBrowser);

  let restoredTabState = JSON.parse(ss.getTabState(tab));
  is(restoredTabState.entries.length, maxBack1 + 1 + maxFwd1,
    "The expected number of session history entries was restored.");
  is(restoredTabState.index, maxBack1 + 1, "The restored tab-state index is correct");

  let indexURLOffset = middleEntry - (restoredTabState.index - 1);
  for (let i = 0; i < restoredTabState.entries.length; i++) {
    is(restoredTabState.entries[i].url, baseURL + (i + indexURLOffset),
        "URL of restored entry matches the expected URL.");
  }

  
  gPrefService.setIntPref("browser.sessionstore.max_serialize_back", maxBack2);
  gPrefService.setIntPref("browser.sessionstore.max_serialize_forward", maxFwd2);

  info("Testing situation where all of the entries in the session history should be restored.");

  ss.setTabState(tab, JSON.stringify(tabState));
  yield promiseTabRestored(tab);
  TabState.flush(tab.linkedBrowser);

  restoredTabState = JSON.parse(ss.getTabState(tab));
  is(restoredTabState.entries.length, maxEntries,
    "The expected number of session history entries was restored.");
  is(restoredTabState.index, middleEntry + 1, "The restored tab-state index is correct");

  for (let i = middleEntry - 2; i <= middleEntry + 2; i++) {
    is(restoredTabState.entries[i].url, baseURL + i,
        "URL of restored entry matches the expected URL.");
  }

  info("Testing situation where only the 1 + maxFwd2 oldest entries should be restored.");

  
  tabState.index = 1;

  ss.setTabState(tab, JSON.stringify(tabState));
  yield promiseTabRestored(tab);
  TabState.flush(tab.linkedBrowser);

  restoredTabState = JSON.parse(ss.getTabState(tab));
  is(restoredTabState.entries.length, 1 + maxFwd2,
    "The expected number of session history entries was restored.");
  is(restoredTabState.index, 1, "The restored tab-state index is correct");

  for (let i = 0; i <= 2; i++) {
    is(restoredTabState.entries[i].url, baseURL + i,
        "URL of restored entry matches the expected URL.");
  }

  info("Testing situation where only the maxBack2 + 1 newest entries should be restored.");

  
  tabState.index = maxEntries;

  ss.setTabState(tab, JSON.stringify(tabState));
  yield promiseTabRestored(tab);
  TabState.flush(tab.linkedBrowser);

  restoredTabState = JSON.parse(ss.getTabState(tab));
  is(restoredTabState.entries.length, maxBack2 + 1,
    "The expected number of session history entries was restored.");
  is(restoredTabState.index, maxBack2 + 1, "The restored tab-state index is correct");

  indexURLOffset = (maxEntries - 1) - maxBack2;
  for (let i = maxBack2 - 2; i <= maxBack2; i++) {
    is(restoredTabState.entries[i].url, baseURL + (i + indexURLOffset),
        "URL of restored entry matches the expected URL.");
  }

  gBrowser.removeTab(tab);
});
