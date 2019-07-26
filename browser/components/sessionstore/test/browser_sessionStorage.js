


"use strict";

let tmp = {};
Cu.import("resource://gre/modules/Promise.jsm", tmp);
let {Promise} = tmp;

const INITIAL_VALUE = "initial-value-" + Date.now();





add_task(function session_storage() {
  let tab = yield createTabWithStorageData(["http://example.com", "http://mochi.test:8888"]);
  let browser = tab.linkedBrowser;

  
  SyncHandlers.get(browser).flush();

  let {storage} = JSON.parse(ss.getTabState(tab));
  is(storage["http://example.com"].test, INITIAL_VALUE,
    "sessionStorage data for example.com has been serialized correctly");
  is(storage["http://mochi.test:8888"].test, INITIAL_VALUE,
    "sessionStorage data for mochi.test has been serialized correctly");

  
  yield modifySessionStorage(browser, {test: "modified"});
  SyncHandlers.get(browser).flush();

  let {storage} = JSON.parse(ss.getTabState(tab));
  is(storage["http://example.com"].test, INITIAL_VALUE,
    "sessionStorage data for example.com has been serialized correctly");
  is(storage["http://mochi.test:8888"].test, "modified",
    "sessionStorage data for mochi.test has been serialized correctly");

  
  let tab2 = gBrowser.duplicateTab(tab);
  let browser2 = tab2.linkedBrowser;
  yield promiseTabRestored(tab2);

  
  SyncHandlers.get(browser2).flush();

  let {storage} = JSON.parse(ss.getTabState(tab2));
  is(storage["http://example.com"].test, INITIAL_VALUE,
    "sessionStorage data for example.com has been duplicated correctly");
  is(storage["http://mochi.test:8888"].test, "modified",
    "sessionStorage data for mochi.test has been duplicated correctly");

  
  
  yield modifySessionStorage(browser2, {test: "modified2"});
  SyncHandlers.get(browser2).flush();

  let {storage} = JSON.parse(ss.getTabState(tab2));
  is(storage["http://example.com"].test, INITIAL_VALUE,
    "sessionStorage data for example.com has been duplicated correctly");
  is(storage["http://mochi.test:8888"].test, "modified2",
    "sessionStorage data for mochi.test has been duplicated correctly");

  
  gBrowser.removeTab(tab);
  gBrowser.removeTab(tab2);
});





add_task(function purge_domain() {
  let tab = yield createTabWithStorageData(["http://example.com", "http://mochi.test:8888"]);
  let browser = tab.linkedBrowser;

  yield notifyObservers(browser, "browser:purge-domain-data", "mochi.test");

  
  SyncHandlers.get(browser).flush();

  let {storage} = JSON.parse(ss.getTabState(tab));
  ok(!storage["http://mochi.test:8888"],
    "sessionStorage data for mochi.test has been purged");
  is(storage["http://example.com"].test, INITIAL_VALUE,
    "sessionStorage data for example.com has been preserved");

  gBrowser.removeTab(tab);
});





add_task(function purge_shistory() {
  let tab = yield createTabWithStorageData(["http://example.com", "http://mochi.test:8888"]);
  let browser = tab.linkedBrowser;

  yield notifyObservers(browser, "browser:purge-session-history");

  
  SyncHandlers.get(browser).flush();

  let {storage} = JSON.parse(ss.getTabState(tab));
  ok(!storage["http://example.com"],
    "sessionStorage data for example.com has been purged");
  is(storage["http://mochi.test:8888"].test, INITIAL_VALUE,
    "sessionStorage data for mochi.test has been preserved");

  gBrowser.removeTab(tab);
});





add_task(function respect_privacy_level() {
  let tab = yield createTabWithStorageData(["http://example.com", "https://example.com"]);
  gBrowser.removeTab(tab);

  let [{state: {storage}}] = JSON.parse(ss.getClosedTabData(window));
  is(storage["http://example.com"].test, INITIAL_VALUE,
    "http sessionStorage data has been saved");
  is(storage["https://example.com"].test, INITIAL_VALUE,
    "https sessionStorage data has been saved");

  
  Services.prefs.setIntPref("browser.sessionstore.privacy_level", 1);

  let tab = yield createTabWithStorageData(["http://example.com", "https://example.com"]);
  gBrowser.removeTab(tab);

  let [{state: {storage}}] = JSON.parse(ss.getClosedTabData(window));
  is(storage["http://example.com"].test, INITIAL_VALUE,
    "http sessionStorage data has been saved");
  ok(!storage["https://example.com"],
    "https sessionStorage data has *not* been saved");

  
  Services.prefs.setIntPref("browser.sessionstore.privacy_level", 2);

  
  let tab = yield createTabWithStorageData(["http://example.com", "https://example.com"]);
  let tab2 = gBrowser.duplicateTab(tab);
  yield promiseBrowserLoaded(tab2.linkedBrowser);
  gBrowser.removeTab(tab);

  
  let [{state: {storage}}] = JSON.parse(ss.getClosedTabData(window));
  ok(!storage["http://example.com"],
    "http sessionStorage data has *not* been saved");
  ok(!storage["https://example.com"],
    "https sessionStorage data has *not* been saved");

  
  Services.prefs.clearUserPref("browser.sessionstore.privacy_level");
  gBrowser.removeTab(tab2);

  
  let [{state: {storage}}] = JSON.parse(ss.getClosedTabData(window));
  is(storage["http://example.com"].test, INITIAL_VALUE,
    "http sessionStorage data has been saved");
  is(storage["https://example.com"].test, INITIAL_VALUE,
    "https sessionStorage data has been saved");
});

function createTabWithStorageData(urls) {
  return Task.spawn(function task() {
    let tab = gBrowser.addTab();
    let browser = tab.linkedBrowser;

    for (let url of urls) {
      browser.loadURI(url);
      yield promiseBrowserLoaded(browser);
      yield modifySessionStorage(browser, {test: INITIAL_VALUE});
    }

    throw new Task.Result(tab);
  });
}

function waitForStorageEvent(browser) {
  return promiseContentMessage(browser, "ss-test:MozStorageChanged");
}

function waitForUpdateMessage(browser) {
  return promiseContentMessage(browser, "SessionStore:update");
}

function modifySessionStorage(browser, data) {
  browser.messageManager.sendAsyncMessage("ss-test:modifySessionStorage", data);
  return waitForStorageEvent(browser);
}

function notifyObservers(browser, topic, data) {
  let msg = {topic: topic, data: data};
  browser.messageManager.sendAsyncMessage("ss-test:notifyObservers", msg);
  return waitForUpdateMessage(browser);
}
