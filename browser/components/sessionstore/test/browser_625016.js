


add_task(function* setup() {
  

  
  
  
  
  
  

  requestLongerTimeout(2);

  yield forceSaveState();

  
  
  while (ss.getClosedWindowCount()) {
    ss.forgetClosedWindow(0);
  }
  is(ss.getClosedWindowCount(), 0, "starting with no closed windows");
});

add_task(function* new_window() {
  let newWin;
  try {
    newWin = yield promiseNewWindowLoaded();
    let tab = newWin.gBrowser.addTab("http://example.com/browser_625016.js?" + Math.random());
    yield promiseBrowserLoaded(tab.linkedBrowser);

    
    is(ss.getClosedWindowCount(), 0, "no closed windows on first save");

    yield promiseWindowClosed(newWin);
    newWin = null;

    let state = JSON.parse((yield promiseRecoveryFileContents()));
    is(state.windows.length, 2,
      "observe1: 2 windows in data written to disk");
    is(state._closedWindows.length, 0,
      "observe1: no closed windows in data written to disk");

    
    is(ss.getClosedWindowCount(), 1,
      "observe1: 1 closed window according to API");
  } finally {
    if (newWin) {
      yield promiseWindowClosed(newWin);
    }
    yield forceSaveState();
  }
});



add_task(function* new_tab() {
  let newTab;
  try {
    newTab = gBrowser.addTab("about:mozilla");

    let state = JSON.parse((yield promiseRecoveryFileContents()));
    is(state.windows.length, 1,
      "observe2: 1 window in data being written to disk");
    is(state._closedWindows.length, 1,
      "observe2: 1 closed window in data being written to disk");

    
    is(ss.getClosedWindowCount(), 1,
      "observe2: 1 closed window according to API");
  } finally {
    gBrowser.removeTab(newTab);
  }
});


add_task(function* done() {
  
  

  while (ss.getClosedWindowCount()) {
    ss.forgetClosedWindow(0);
  }
  Services.prefs.clearUserPref("browser.sessionstore.interval");
});
