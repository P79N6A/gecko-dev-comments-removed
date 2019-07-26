





let closedWindowCount = 0;

let now = Date.now();

const TESTS = [
  { url: "about:config",
    key: "bug 394759 Non-PB",
    value: "uniq" + (++now) },
  { url: "about:mozilla",
    key: "bug 394759 PB",
    value: "uniq" + (++now) },
];

function promiseTestOpenCloseWindow(aIsPrivate, aTest) {
  return Task.spawn(function*() {
    let win = yield promiseNewWindowLoaded({ "private": aIsPrivate });
    win.gBrowser.selectedBrowser.loadURI(aTest.url);
    yield promiseBrowserLoaded(win.gBrowser.selectedBrowser);
    yield Promise.resolve();
    
    ss.setWindowValue(win, aTest.key, aTest.value);
    
    yield promiseWindowClosed(win);
  });
}

function promiseTestOnWindow(aIsPrivate, aValue) {
  return Task.spawn(function*() {
    let win = yield promiseNewWindowLoaded({ "private": aIsPrivate });
    yield promiseCheckClosedWindows(aIsPrivate, aValue);
    registerCleanupFunction(() => promiseWindowClosed(win));
  });
}

function promiseCheckClosedWindows(aIsPrivate, aValue) {
  return Task.spawn(function*() {
    let data = JSON.parse(ss.getClosedWindowData())[0];
    is(ss.getClosedWindowCount(), 1, "Check that the closed window count hasn't changed");
    ok(JSON.stringify(data).indexOf(aValue) > -1,
       "Check the closed window data was stored correctly");
  });
}

function promiseBlankState() {
  return Task.spawn(function*() {
    
    
    Services.prefs.setIntPref("browser.sessionstore.interval", 100000);
    registerCleanupFunction(() =>  Services.prefs.clearUserPref("browser.sessionstore.interval"));

    
    
    let blankState = JSON.stringify({
      windows: [{
        tabs: [{ entries: [{ url: "about:blank" }] }],
        _closedTabs: []
      }],
      _closedWindows: []
    });

    ss.setBrowserState(blankState);

    
    
    

    yield forceSaveState();
    closedWindowCount = ss.getClosedWindowCount();
    is(closedWindowCount, 0, "Correctly set window count");

    
    yield SessionFile.wipe();

    
    
    yield forceSaveState();
  });
}

add_task(function* init() {
  while (ss.getClosedWindowCount() > 0) {
    ss.forgetClosedWindow(0);
  }
  while (ss.getClosedTabCount(window) > 0) {
    ss.forgetClosedTab(window, 0);
  }
});

add_task(function* main() {
  yield promiseTestOpenCloseWindow(false, TESTS[0]);
  yield promiseTestOpenCloseWindow(true, TESTS[1]);
  yield promiseTestOnWindow(false, TESTS[0].value);
  yield promiseTestOnWindow(true, TESTS[0].value);
});

