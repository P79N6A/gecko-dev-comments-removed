


function test() {
  TestRunner.run();
}







function runTests() {
  
  let win = OpenBrowserWindow();
  yield whenDelayedStartupFinished(win, next);

  
  let flags = Ci.nsIWebNavigation.LOAD_FLAGS_REPLACE_HISTORY;
  win.gBrowser.selectedBrowser.loadURIWithFlags("about:robots", flags);
  yield whenBrowserLoaded(win.gBrowser.selectedBrowser);

  
  let tab = win.gBrowser.addTab("about:mozilla");
  yield whenBrowserLoaded(tab.linkedBrowser);
  TabState.flush(tab.linkedBrowser);
  win.gBrowser.removeTab(win.gBrowser.tabs[0]);

  
  
  ok("__SSi" in win, "window is being tracked by sessionstore");
  ss.setWindowValue(win, "foo", "bar");
  checkWindowState(win);

  let state = ss.getWindowState(win);
  let closedTabData = ss.getClosedTabData(win);

  
  whenWindowClosed(win);
  yield win.close();

  
  
  ok(!("__SSi" in win), "sessionstore does no longer track our window");
  checkWindowState(win);

  
  ok(shouldThrow(() => ss.setWindowState(win, {})),
     "we're not allowed to modify state data anymore");
  ok(shouldThrow(() => ss.setWindowValue(win, "foo", "baz")),
     "we're not allowed to modify state data anymore");
}

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

function whenWindowClosed(window) {
  window.addEventListener("SSWindowClosing", function onClosing() {
    window.removeEventListener("SSWindowClosing", onClosing);
    executeSoon(next);
  });
}
