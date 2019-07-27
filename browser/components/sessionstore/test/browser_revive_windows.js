


"use strict";

const IS_MAC = ("nsILocalFileMac" in Ci);
const URL_PREFIX = "about:mozilla?t=browser_revive_windows&r=";
const PREF_MAX_UNDO = "browser.sessionstore.max_windows_undo";

const URL_MAIN_WINDOW = URL_PREFIX + Math.random();
const URL_ADD_WINDOW1 = URL_PREFIX + Math.random();
const URL_ADD_WINDOW2 = URL_PREFIX + Math.random();
const URL_CLOSED_WINDOW = URL_PREFIX + Math.random();

add_task(function* setup() {
  registerCleanupFunction(() => Services.prefs.clearUserPref(PREF_MAX_UNDO));
});






add_task(function* test_revive_windows() {
  
  Services.prefs.setIntPref(PREF_MAX_UNDO, 1);

  
  forgetClosedWindows();

  let windows = [];

  
  for (let i = 0; i < 3; i++) {
    let win = yield promiseNewWindow();
    windows.push(win);

    let tab = win.gBrowser.addTab("about:mozilla");
    yield promiseBrowserLoaded(tab.linkedBrowser);
  }

  
  for (let win of windows) {
    yield promiseWindowClosed(win);
  }

  is(ss.getClosedWindowCount(), 1, "one window restorable");

  
  let state = JSON.parse(yield promiseRecoveryFileContents());

  
  if (IS_MAC) {
    is(state.windows.length, 1, "one open window");
    is(state._closedWindows.length, 1, "one closed window");
  } else {
    is(state.windows.length, 4, "four open windows");
    is(state._closedWindows.length, 0, "closed windows");
  }
});






add_task(function* test_revive_windows_order() {
  
  Services.prefs.setIntPref(PREF_MAX_UNDO, 3);

  
  forgetClosedWindows();

  let tab = gBrowser.addTab(URL_MAIN_WINDOW);
  yield promiseBrowserLoaded(tab.linkedBrowser);
  registerCleanupFunction(() => gBrowser.removeTab(tab));

  let win0 = yield promiseNewWindow();
  let tab0 = win0.gBrowser.addTab(URL_CLOSED_WINDOW);
  yield promiseBrowserLoaded(tab0.linkedBrowser);

  yield promiseWindowClosed(win0);
  let data = ss.getClosedWindowData();
  ok(data.contains(URL_CLOSED_WINDOW), "window is restorable");

  let win1 = yield promiseNewWindow();
  let tab1 = win1.gBrowser.addTab(URL_ADD_WINDOW1);
  yield promiseBrowserLoaded(tab1.linkedBrowser);

  let win2 = yield promiseNewWindow();
  let tab2 = win2.gBrowser.addTab(URL_ADD_WINDOW2);
  yield promiseBrowserLoaded(tab2.linkedBrowser);

  
  
  yield promiseWindowClosed(win1);
  yield promiseWindowClosed(win2);

  
  for (let i = 0; i < 2; i++) {
    info(`checking window data, iteration #${i}`);
    let contents = yield promiseRecoveryFileContents();
    let {windows, _closedWindows: closedWindows} = JSON.parse(contents);

    if (IS_MAC) {
      
      is(windows.length, 1, "one open window");
      is(closedWindows.length, 3, "three closed windows");

      
      ok(JSON.stringify(windows).contains(URL_MAIN_WINDOW),
        "open window is correct");

      
      ok(JSON.stringify(closedWindows[0]).contains(URL_ADD_WINDOW2),
        "correct first additional window");
      ok(JSON.stringify(closedWindows[1]).contains(URL_ADD_WINDOW1),
        "correct second additional window");
      ok(JSON.stringify(closedWindows[2]).contains(URL_CLOSED_WINDOW),
        "correct main window");
    } else {
      
      is(windows.length, 3, "three open windows");
      is(closedWindows.length, 1, "one closed window");

      
      ok(JSON.stringify(closedWindows).contains(URL_CLOSED_WINDOW),
        "closed window is correct");

      
      ok(JSON.stringify(windows[0]).contains(URL_ADD_WINDOW1),
        "correct first additional window");
      ok(JSON.stringify(windows[1]).contains(URL_ADD_WINDOW2),
        "correct second additional window");
      ok(JSON.stringify(windows[2]).contains(URL_MAIN_WINDOW),
        "correct main window");
    }
  }
});

function promiseNewWindow() {
  return new Promise(resolve => whenNewWindowLoaded({private: false}, resolve));
}

function forgetClosedWindows() {
  while (ss.getClosedWindowCount()) {
    ss.forgetClosedWindow(0);
  }
}
