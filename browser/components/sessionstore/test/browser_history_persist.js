


"use strict";





add_task(function check_history_not_persisted() {
  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  TabState.flush(browser);
  let state = JSON.parse(ss.getTabState(tab));
  ok(!state.entries[0].persist, "Should have collected the persistence state");
  gBrowser.removeTab(tab);
  browser = null;

  
  tab = gBrowser.addTab("about:blank");
  browser = tab.linkedBrowser;
  yield promiseTabState(tab, state);
  let sessionHistory = browser.sessionHistory;

  is(sessionHistory.count, 1, "Should be a single history entry");
  is(sessionHistory.getEntryAtIndex(0, false).URI.spec, "about:blank", "Should be the right URL");

  
  browser.loadURI("about:robots");
  yield promiseBrowserLoaded(browser);
  sessionHistory = browser.sessionHistory;
  is(sessionHistory.count, 1, "Should be a single history entry");
  is(sessionHistory.getEntryAtIndex(0, false).URI.spec, "about:robots", "Should be the right URL");

  
  gBrowser.removeTab(tab);
});





add_task(function check_history_default_persisted() {
  
  let tab = gBrowser.addTab("about:blank");
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  TabState.flush(browser);
  let state = JSON.parse(ss.getTabState(tab));
  delete state.entries[0].persist;
  gBrowser.removeTab(tab);
  browser = null;

  
  tab = gBrowser.addTab("about:blank");
  browser = tab.linkedBrowser;
  yield promiseTabState(tab, state);
  let sessionHistory = browser.sessionHistory;

  is(sessionHistory.count, 1, "Should be a single history entry");
  is(sessionHistory.getEntryAtIndex(0, false).URI.spec, "about:blank", "Should be the right URL");

  
  browser.loadURI("about:robots");
  yield promiseBrowserLoaded(browser);
  sessionHistory = browser.sessionHistory;
  is(sessionHistory.count, 2, "Should be two history entries");
  is(sessionHistory.getEntryAtIndex(0, false).URI.spec, "about:blank", "Should be the right URL");
  is(sessionHistory.getEntryAtIndex(1, false).URI.spec, "about:robots", "Should be the right URL");

  
  gBrowser.removeTab(tab);
});
