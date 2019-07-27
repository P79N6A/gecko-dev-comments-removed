


"use strict";

const RAND = Math.random();
const URL = "http://mochi.test:8888/browser/" +
            "browser/components/sessionstore/test/browser_sessionStorage.html" +
            "?" + RAND;

const OUTER_VALUE = "outer-value-" + RAND;
const INNER_VALUE = "inner-value-" + RAND;





add_task(function session_storage() {
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield TabStateFlusher.flush(browser);

  let {storage} = JSON.parse(ss.getTabState(tab));
  is(storage["http://example.com"].test, INNER_VALUE,
    "sessionStorage data for example.com has been serialized correctly");
  is(storage["http://mochi.test:8888"].test, OUTER_VALUE,
    "sessionStorage data for mochi.test has been serialized correctly");

  
  yield modifySessionStorage(browser, {test: "modified1"}, {frameIndex: 0});
  yield TabStateFlusher.flush(browser);

  ({storage} = JSON.parse(ss.getTabState(tab)));
  is(storage["http://example.com"].test, "modified1",
    "sessionStorage data for example.com has been serialized correctly");
  is(storage["http://mochi.test:8888"].test, OUTER_VALUE,
    "sessionStorage data for mochi.test has been serialized correctly");

  
  yield modifySessionStorage(browser, {test: "modified"});
  yield modifySessionStorage(browser, {test: "modified2"}, {frameIndex: 0});
  yield TabStateFlusher.flush(browser);

  ({storage} = JSON.parse(ss.getTabState(tab)));
  is(storage["http://example.com"].test, "modified2",
    "sessionStorage data for example.com has been serialized correctly");
  is(storage["http://mochi.test:8888"].test, "modified",
    "sessionStorage data for mochi.test has been serialized correctly");

  
  let tab2 = gBrowser.duplicateTab(tab);
  let browser2 = tab2.linkedBrowser;
  yield promiseTabRestored(tab2);

  
  yield TabStateFlusher.flush(browser2);

  ({storage} = JSON.parse(ss.getTabState(tab2)));
  is(storage["http://example.com"].test, "modified2",
    "sessionStorage data for example.com has been duplicated correctly");
  is(storage["http://mochi.test:8888"].test, "modified",
    "sessionStorage data for mochi.test has been duplicated correctly");

  
  
  yield modifySessionStorage(browser2, {test: "modified3"});
  yield TabStateFlusher.flush(browser2);

  ({storage} = JSON.parse(ss.getTabState(tab2)));
  is(storage["http://example.com"].test, "modified2",
    "sessionStorage data for example.com has been duplicated correctly");
  is(storage["http://mochi.test:8888"].test, "modified3",
    "sessionStorage data for mochi.test has been duplicated correctly");

  
  browser2.loadURI("http://mochi.test:8888/");
  yield promiseBrowserLoaded(browser2);
  yield TabStateFlusher.flush(browser2);

  ({storage} = JSON.parse(ss.getTabState(tab2)));
  is(storage["http://mochi.test:8888"].test, "modified3",
    "navigating retains correct storage data");
  ok(!storage["http://example.com"], "storage data was discarded");

  
  browser2.loadURI("about:mozilla");
  yield promiseBrowserLoaded(browser2);
  yield TabStateFlusher.flush(browser2);

  let state = JSON.parse(ss.getTabState(tab2));
  ok(!state.hasOwnProperty("storage"), "storage data was discarded");

  
  yield promiseRemoveTab(tab);
  yield promiseRemoveTab(tab2);
});





add_task(function purge_domain() {
  let tab = gBrowser.addTab(URL);
  let browser = tab.linkedBrowser;
  yield promiseBrowserLoaded(browser);

  
  yield purgeDomainData(browser, "mochi.test");

  
  yield TabStateFlusher.flush(browser);

  let {storage} = JSON.parse(ss.getTabState(tab));
  ok(!storage["http://mochi.test:8888"],
    "sessionStorage data for mochi.test has been purged");
  is(storage["http://example.com"].test, INNER_VALUE,
    "sessionStorage data for example.com has been preserved");

  yield promiseRemoveTab(tab);
});





add_task(function respect_privacy_level() {
  let tab = gBrowser.addTab(URL + "&secure");
  yield promiseBrowserLoaded(tab.linkedBrowser);
  yield promiseRemoveTab(tab);

  let [{state: {storage}}] = JSON.parse(ss.getClosedTabData(window));
  is(storage["http://mochi.test:8888"].test, OUTER_VALUE,
    "http sessionStorage data has been saved");
  is(storage["https://example.com"].test, INNER_VALUE,
    "https sessionStorage data has been saved");

  
  Services.prefs.setIntPref("browser.sessionstore.privacy_level", 1);

  tab = gBrowser.addTab(URL + "&secure");
  yield promiseBrowserLoaded(tab.linkedBrowser);
  yield promiseRemoveTab(tab);

  [{state: {storage}}] = JSON.parse(ss.getClosedTabData(window));
  is(storage["http://mochi.test:8888"].test, OUTER_VALUE,
    "http sessionStorage data has been saved");
  ok(!storage["https://example.com"],
    "https sessionStorage data has *not* been saved");

  
  Services.prefs.setIntPref("browser.sessionstore.privacy_level", 2);

  
  tab = gBrowser.addTab(URL + "&secure");
  yield promiseBrowserLoaded(tab.linkedBrowser);
  let tab2 = gBrowser.duplicateTab(tab);
  yield promiseTabRestored(tab2);
  yield promiseRemoveTab(tab);

  
  [{state: {storage}}] = JSON.parse(ss.getClosedTabData(window));
  ok(!storage, "sessionStorage data has *not* been saved");

  
  
  
  while (ss.getClosedTabCount(window) > 0) {
    ss.forgetClosedTab(window, 0);
  }

  
  Services.prefs.clearUserPref("browser.sessionstore.privacy_level");
  yield promiseRemoveTab(tab2);

  
  [{state: {storage}}] = JSON.parse(ss.getClosedTabData(window));
  is(storage["http://mochi.test:8888"].test, OUTER_VALUE,
    "http sessionStorage data has been saved");
  is(storage["https://example.com"].test, INNER_VALUE,
    "https sessionStorage data has been saved");
});

function purgeDomainData(browser, domain) {
  return sendMessage(browser, "ss-test:purgeDomainData", domain);
}
