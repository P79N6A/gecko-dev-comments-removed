"use strict";







add_task(function* test() {
  
  let win = yield promiseNewWindowLoaded();

  
  let flags = Ci.nsIWebNavigation.LOAD_FLAGS_REPLACE_HISTORY;
  win.gBrowser.selectedBrowser.loadURIWithFlags("about:robots", flags);
  yield promiseBrowserLoaded(win.gBrowser.selectedBrowser);

  
  let tab = win.gBrowser.addTab("about:mozilla");
  yield promiseBrowserLoaded(tab.linkedBrowser);
  yield TabStateFlusher.flush(tab.linkedBrowser);
  yield promiseRemoveTab(win.gBrowser.tabs[0]);

  
  
  ok("__SSi" in win, "window is being tracked by sessionstore");
  ss.setWindowValue(win, "foo", "bar");
  checkWindowState(win);

  let state = ss.getWindowState(win);
  let closedTabData = ss.getClosedTabData(win);

  
  yield promiseWindowClosed(win);

  
  
  ok(!("__SSi" in win), "sessionstore does no longer track our window");
  checkWindowState(win);

  
  Assert.throws(() => ss.setWindowState(win, {}),
    "we're not allowed to modify state data anymore");
  Assert.throws(() => ss.setWindowValue(win, "foo", "baz"),
    "we're not allowed to modify state data anymore");
});

function checkWindowState(window) {
  let {windows: [{tabs}]} = JSON.parse(ss.getWindowState(window));
  is(tabs.length, 1, "the window has a single tab");
  is(tabs[0].entries[0].url, "about:mozilla", "the tab is about:mozilla");

  is(ss.getClosedTabCount(window), 1, "the window has one closed tab");
  let [{state: {entries: [{url}]}}] = JSON.parse(ss.getClosedTabData(window));
  is(url, "about:robots", "the closed tab is about:robots");

  is(ss.getWindowValue(window, "foo"), "bar", "correct extData value");
}

function shouldThrow(f) {
  try {
    f();
  } catch (e) {
    return true;
  }
}
