


"use strict";

const INITIAL_VALUE = "browser_broadcast.js-initial-value-" + Date.now();





add_task(function flush_on_tabclose() {
  let tab = yield createTabWithStorageData(["http://example.com"]);
  let browser = tab.linkedBrowser;

  yield modifySessionStorage(browser, {test: "on-tab-close"});
  yield promiseRemoveTab(tab);

  let [{state: {storage}}] = JSON.parse(ss.getClosedTabData(window));
  is(storage["http://example.com"].test, "on-tab-close",
    "sessionStorage data has been flushed on TabClose");
});





add_task(function flush_on_quit_requested() {
  let tab = yield createTabWithStorageData(["http://example.com"]);
  let browser = tab.linkedBrowser;

  yield modifySessionStorage(browser, {test: "on-quit-requested"});

  
  
  
  
  sendQuitApplicationRequested();

  let {storage} = JSON.parse(ss.getTabState(tab));
  is(storage["http://example.com"].test, "on-quit-requested",
    "sessionStorage data has been flushed when a quit is requested");

  gBrowser.removeTab(tab);
});





add_task(function flush_on_duplicate() {
  let tab = yield createTabWithStorageData(["http://example.com"]);
  let browser = tab.linkedBrowser;

  yield modifySessionStorage(browser, {test: "on-duplicate"});
  let tab2 = ss.duplicateTab(window, tab);
  yield promiseTabRestored(tab2);

  yield promiseRemoveTab(tab2);
  let [{state: {storage}}] = JSON.parse(ss.getClosedTabData(window));
  is(storage["http://example.com"].test, "on-duplicate",
    "sessionStorage data has been flushed when duplicating tabs");

  gBrowser.removeTab(tab);
});





add_task(function flush_on_windowclose() {
  let win = yield promiseNewWindow();
  let tab = yield createTabWithStorageData(["http://example.com"], win);
  let browser = tab.linkedBrowser;

  yield modifySessionStorage(browser, {test: "on-window-close"});
  yield closeWindow(win);

  let [{tabs: [_, {storage}]}] = JSON.parse(ss.getClosedWindowData());
  is(storage["http://example.com"].test, "on-window-close",
    "sessionStorage data has been flushed when closing a window");
});





add_task(function flush_on_settabstate() {
  let tab = yield createTabWithStorageData(["http://example.com"]);
  let browser = tab.linkedBrowser;

  
  yield TabStateFlusher.flush(browser);

  let state = ss.getTabState(tab);
  yield modifySessionStorage(browser, {test: "on-set-tab-state"});

  
  
  TabState.flushAsync(browser);

  yield promiseTabState(tab, state);

  let {storage} = JSON.parse(ss.getTabState(tab));
  is(storage["http://example.com"].test, INITIAL_VALUE,
    "sessionStorage data has not been overwritten");

  gBrowser.removeTab(tab);
});






add_task(function flush_on_tabclose_racy() {
  let tab = yield createTabWithStorageData(["http://example.com"]);
  let browser = tab.linkedBrowser;

  
  yield TabStateFlusher.flush(browser);

  yield modifySessionStorage(browser, {test: "on-tab-close-racy"});

  
  
  TabState.flushAsync(browser);
  yield promiseRemoveTab(tab);

  let [{state: {storage}}] = JSON.parse(ss.getClosedTabData(window));
  is(storage["http://example.com"].test, "on-tab-close-racy",
    "sessionStorage data has been merged correctly to prevent data loss");
});

function promiseNewWindow() {
  let deferred = Promise.defer();
  whenNewWindowLoaded({private: false}, deferred.resolve);
  return deferred.promise;
}

function closeWindow(win) {
  let deferred = Promise.defer();
  let outerID = win.QueryInterface(Ci.nsIInterfaceRequestor)
                   .getInterface(Ci.nsIDOMWindowUtils)
                   .outerWindowID;

  Services.obs.addObserver(function obs(subject, topic) {
    let id = subject.QueryInterface(Ci.nsISupportsPRUint64).data;
    if (id == outerID) {
      Services.obs.removeObserver(obs, topic);
      deferred.resolve();
    }
  }, "outer-window-destroyed", false);

  win.close();
  return deferred.promise;
}

function createTabWithStorageData(urls, win = window) {
  return Task.spawn(function task() {
    let tab = win.gBrowser.addTab();
    let browser = tab.linkedBrowser;

    for (let url of urls) {
      browser.loadURI(url);
      yield promiseBrowserLoaded(browser);
      yield modifySessionStorage(browser, {test: INITIAL_VALUE});
    }

    throw new Task.Result(tab);
  });
}

function sendQuitApplicationRequested() {
  let cancelQuit = Cc["@mozilla.org/supports-PRBool;1"]
                     .createInstance(Ci.nsISupportsPRBool);
  Services.obs.notifyObservers(cancelQuit, "quit-application-requested", null);
}
