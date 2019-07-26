


const originalState = ss.getBrowserState();


function test() {
  waitForExplicitFinish();

  registerCleanupFunction(function() {
    Services.prefs.clearUserPref("browser.sessionstore.interval");
    ss.setBrowserState(originalState);
  });

  runNextTest();
}

let tests = [test_1, test_2, test_3 ];

const testState = {
  windows: [{
    tabs: [
      { entries: [{ url: "about:blank" }] },
    ]
  }]
};

function runNextTest() {
  
  closeAllButPrimaryWindow();

  
  if (tests.length) {
    let currentTest = tests.shift();
    waitForBrowserState(testState, currentTest);
  } else {
    Services.prefs.clearUserPref("browser.sessionstore.interval");
    ss.setBrowserState(originalState);
    finish();
  }
}



function test_1() {
  testOnWindow(false, function(aWindow) {
    aWindow.gBrowser.addTab("http://www.example.com/1");
    testOnWindow(true, function(aWindow) {
      aWindow.gBrowser.addTab("http://www.example.com/2");
      testOnWindow(false, function(aWindow) {
        aWindow.gBrowser.addTab("http://www.example.com/3");
        testOnWindow(true, function(aWindow) {
          aWindow.gBrowser.addTab("http://www.example.com/4");

          let curState = JSON.parse(ss.getBrowserState());
          is (curState.windows.length, 5, "Browser has opened 5 windows");
          is (curState.windows[2].isPrivate, true, "Window is private");
          is (curState.windows[4].isPrivate, true, "Last window is private");
          is (curState.selectedWindow, 5, "Last window opened is the one selected");

          forceWriteState(function(state) {
            is(state.windows.length, 3,
               "sessionstore state: 3 windows in data being written to disk");
            is (state.selectedWindow, 3,
               "Selected window is updated to match one of the saved windows");
            state.windows.forEach(function(win) {
              is(!win.isPrivate, true, "Saved window is not private");
            });
            is(state._closedWindows.length, 0,
               "sessionstore state: no closed windows in data being written to disk");
            runNextTest();
          });
        });
      });
    });
  });
}


function test_2() {
  testOnWindow(true, function(aWindow) {
    aWindow.gBrowser.addTab("http://www.example.com/1");
    testOnWindow(true, function(aWindow) {
      aWindow.gBrowser.addTab("http://www.example.com/2");

      let curState = JSON.parse(ss.getBrowserState());
      is (curState.windows.length, 3, "Browser has opened 3 windows");
      is (curState.windows[1].isPrivate, true, "Window 1 is private");
      is (curState.windows[2].isPrivate, true, "Window 2 is private");
      is (curState.selectedWindow, 3, "Last window opened is the one selected");

      forceWriteState(function(state) {
        is(state.windows.length, 1,
           "sessionstore state: 1 windows in data being writted to disk");
        is (state.selectedWindow, 1,
           "Selected window is updated to match one of the saved windows");
        is(state._closedWindows.length, 0,
           "sessionstore state: no closed windows in data being writted to disk");
        runNextTest();
      });
    });
  });
}


function test_3() {
  testOnWindow(false, function(normalWindow) {
    let tab = normalWindow.gBrowser.addTab("http://www.example.com/1");
    whenBrowserLoaded(tab.linkedBrowser, function() {
      testOnWindow(true, function(aWindow) {
        aWindow.gBrowser.addTab("http://www.example.com/2");
        testOnWindow(false, function(aWindow) {
          aWindow.gBrowser.addTab("http://www.example.com/3");

          let curState = JSON.parse(ss.getBrowserState());
          is (curState.windows.length, 4, "Browser has opened 4 windows");
          is (curState.windows[2].isPrivate, true, "Window 2 is private");
          is (curState.selectedWindow, 4, "Last window opened is the one selected");

          waitForWindowClose(normalWindow, function() {
            forceWriteState(function(state) {
              is(state.windows.length, 2,
                 "sessionstore state: 2 windows in data being writted to disk");
              is(state.selectedWindow, 2,
                 "Selected window is updated to match one of the saved windows");
              state.windows.forEach(function(win) {
                is(!win.isPrivate, true, "Saved window is not private");
              });
              is(state._closedWindows.length, 1,
                 "sessionstore state: 1 closed window in data being writted to disk");
              state._closedWindows.forEach(function(win) {
                is(!win.isPrivate, true, "Closed window is not private");
              });
              runNextTest();
            });
          });
        });
      });
    });
  });
}

function waitForWindowClose(aWin, aCallback) {
  Services.obs.addObserver(function observe(aSubject, aTopic, aData) {
    if (aTopic == "domwindowclosed" && aWin == aSubject) {
      Services.obs.removeObserver(observe, aTopic);
      checkWindowIsClosed(aWin, aCallback);
    }
  }, "domwindowclosed", false);
  aWin.close();
}

function checkWindowIsClosed(aWin, aCallback) {
  if (aWin.closed) {
    info("Window is closed");
    executeSoon(aCallback);
  } else {
    executeSoon(function() {
      checkWindowIsClosed(aWin, aCallback);
    });
  }
}

function forceWriteState(aCallback) {
  Services.obs.addObserver(function observe(aSubject, aTopic, aData) {
    if (aTopic == "sessionstore-state-write") {
      Services.obs.removeObserver(observe, aTopic);
      aSubject.QueryInterface(Ci.nsISupportsString);
      aCallback(JSON.parse(aSubject.data));
    }
  }, "sessionstore-state-write", false);
  Services.prefs.setIntPref("browser.sessionstore.interval", 0);
}

function testOnWindow(aIsPrivate, aCallback) {
  let win = OpenBrowserWindow({private: aIsPrivate});
  win.addEventListener("load", function onLoad() {
    win.removeEventListener("load", onLoad, false);
    executeSoon(function() { aCallback(win); });
  }, false);
}
