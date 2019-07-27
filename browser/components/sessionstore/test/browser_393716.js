


"use strict";

const URL = "about:config";




add_task(function test_set_tabstate() {
  let key = "Unique key: " + Date.now();
  let value = "Unique value: " + Math.random();

  
  let tab = gBrowser.addTab(URL);
  ss.setTabValue(tab, key, value);
  yield promiseBrowserLoaded(tab.linkedBrowser);

  
  TabState.flush(tab.linkedBrowser);
  let state = ss.getTabState(tab);
  ok(state, "get the tab's state");

  
  state = JSON.parse(state);
  ok(state instanceof Object && state.entries instanceof Array && state.entries.length > 0,
     "state object seems valid");
  ok(state.entries.length == 1 && state.entries[0].url == URL,
     "Got the expected state object (test URL)");
  ok(state.extData && state.extData[key] == value,
     "Got the expected state object (test manually set tab value)");

  
  gBrowser.removeTab(tab);
});

add_task(function test_set_tabstate_and_duplicate() {
  let key2 = "key2";
  let value2 = "Value " + Math.random();
  let value3 = "Another value: " + Date.now();
  let state = { entries: [{ url: URL }], extData: { key2: value2 } };

  
  let tab = gBrowser.addTab();
  
  ss.setTabState(tab, JSON.stringify(state));
  yield promiseBrowserLoaded(tab.linkedBrowser);

  
  ok(ss.getTabValue(tab, key2) == value2 && tab.linkedBrowser.currentURI.spec == URL,
     "the tab's state was correctly restored");

  
  yield setInputValue(tab.linkedBrowser, {id: "textbox", value: value3});

  
  let tab2 = ss.duplicateTab(window, tab);
  yield promiseTabRestored(tab2);

  
  ok(ss.getTabValue(tab2, key2) == value2 &&
     tab2.linkedBrowser.currentURI.spec == URL,
     "correctly duplicated the tab's state");
  let textbox = yield getInputValue(tab2.linkedBrowser, {id: "textbox"});
  is(textbox, value3, "also duplicated text data");

  
  gBrowser.removeTab(tab2);
  gBrowser.removeTab(tab);
});
