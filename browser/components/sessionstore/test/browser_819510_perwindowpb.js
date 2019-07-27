




add_task(function* test_1() {
  let win = yield promiseNewWindowLoaded();
  win.gBrowser.addTab("http://www.example.com/1");

  win = yield promiseNewWindowLoaded({private: true});
  win.gBrowser.addTab("http://www.example.com/2");

  win = yield promiseNewWindowLoaded();
  win.gBrowser.addTab("http://www.example.com/3");

  win = yield promiseNewWindowLoaded({private: true});
  win.gBrowser.addTab("http://www.example.com/4");

  let curState = JSON.parse(ss.getBrowserState());
  is(curState.windows.length, 5, "Browser has opened 5 windows");
  is(curState.windows[2].isPrivate, true, "Window is private");
  is(curState.windows[4].isPrivate, true, "Last window is private");
  is(curState.selectedWindow, 5, "Last window opened is the one selected");

  let state = JSON.parse(yield promiseRecoveryFileContents());

  is(state.windows.length, 3,
     "sessionstore state: 3 windows in data being written to disk");
  is(state.selectedWindow, 3,
     "Selected window is updated to match one of the saved windows");
  ok(state.windows.every(win => !win.isPrivate),
    "Saved windows are not private");
  is(state._closedWindows.length, 0,
     "sessionstore state: no closed windows in data being written to disk");

  
  yield promiseAllButPrimaryWindowClosed();
  forgetClosedWindows();
});


add_task(function* test_2() {
  let win = yield promiseNewWindowLoaded({private: true});
  win.gBrowser.addTab("http://www.example.com/1");

  win = yield promiseNewWindowLoaded({private: true});
  win.gBrowser.addTab("http://www.example.com/2");

  let curState = JSON.parse(ss.getBrowserState());
  is(curState.windows.length, 3, "Browser has opened 3 windows");
  is(curState.windows[1].isPrivate, true, "Window 1 is private");
  is(curState.windows[2].isPrivate, true, "Window 2 is private");
  is(curState.selectedWindow, 3, "Last window opened is the one selected");

  let state = JSON.parse(yield promiseRecoveryFileContents());

  is(state.windows.length, 1,
     "sessionstore state: 1 windows in data being written to disk");
  is(state.selectedWindow, 1,
     "Selected window is updated to match one of the saved windows");
  is(state._closedWindows.length, 0,
     "sessionstore state: no closed windows in data being written to disk");

  
  yield promiseAllButPrimaryWindowClosed();
  forgetClosedWindows();
});


add_task(function* test_3() {
  let normalWindow = yield promiseNewWindowLoaded();
  yield promiseTabLoad(normalWindow, "http://www.example.com/");

  let win = yield promiseNewWindowLoaded({private: true});
  yield promiseTabLoad(win, "http://www.example.com/");

  win = yield promiseNewWindowLoaded();
  yield promiseTabLoad(win, "http://www.example.com/");

  let curState = JSON.parse(ss.getBrowserState());
  is(curState.windows.length, 4, "Browser has opened 4 windows");
  is(curState.windows[2].isPrivate, true, "Window 2 is private");
  is(curState.selectedWindow, 4, "Last window opened is the one selected");

  yield promiseWindowClosed(normalWindow);

  
  
  
  let tab = win.gBrowser.tabs[0];
  win.gBrowser.pinTab(tab);
  win.gBrowser.unpinTab(tab);

  let state = JSON.parse(yield promiseRecoveryFileContents());

  is(state.windows.length, 2,
     "sessionstore state: 2 windows in data being written to disk");
  is(state.selectedWindow, 2,
     "Selected window is updated to match one of the saved windows");
  ok(state.windows.every(win => !win.isPrivate),
    "Saved windows are not private");
  is(state._closedWindows.length, 1,
     "sessionstore state: 1 closed window in data being written to disk");
  ok(state._closedWindows.every(win => !win.isPrivate),
    "Closed windows are not private");

  
  yield promiseAllButPrimaryWindowClosed();
  forgetClosedWindows();
});

function promiseTabLoad(win, url) {
  let browser = win.gBrowser.selectedBrowser;
  browser.loadURI(url);
  return promiseBrowserLoaded(browser).then(() => TabState.flush(browser));
}
