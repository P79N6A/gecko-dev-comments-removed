


const originalState = ss.getBrowserState();


function test() {
  waitForExplicitFinish();
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
    Services.obs.addObserver(
      function observe(aSubject, aTopic, aData) {
        Services.obs.removeObserver(observe, aTopic);
        finish();
      },
      "sessionstore-browser-state-restored", false);
    ss.setBrowserState(originalState);
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
           "sessionstore state: 1 windows in data being written to disk");
        is (state.selectedWindow, 1,
           "Selected window is updated to match one of the saved windows");
        is(state._closedWindows.length, 0,
           "sessionstore state: no closed windows in data being written to disk");
        runNextTest();
      });
    });
  });
}


function test_3() {
  testOnWindow(false, function(normalWindow) {
    waitForTabLoad(normalWindow, "http://www.example.com/", function() {
      testOnWindow(true, function(aWindow) {
        waitForTabLoad(aWindow, "http://www.example.com/", function() {
          testOnWindow(false, function(aWindow) {
            waitForTabLoad(aWindow, "http://www.example.com/", function() {

              let curState = JSON.parse(ss.getBrowserState());
              is(curState.windows.length, 4, "Browser has opened 4 windows");
              is(curState.windows[2].isPrivate, true, "Window 2 is private");
              is(curState.selectedWindow, 4, "Last window opened is the one selected");

              waitForWindowClose(normalWindow, function() {
                
                
                
                let tab = aWindow.gBrowser.tabs[0];
                aWindow.gBrowser.pinTab(tab);
                aWindow.gBrowser.unpinTab(tab);

                forceWriteState(function(state) {
                  is(state.windows.length, 2,
                     "sessionstore state: 2 windows in data being written to disk");
                  is(state.selectedWindow, 2,
                     "Selected window is updated to match one of the saved windows");
                  state.windows.forEach(function(win) {
                    is(!win.isPrivate, true, "Saved window is not private");
                  });
                  is(state._closedWindows.length, 1,
                     "sessionstore state: 1 closed window in data being written to disk");
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
    });
  });
}

function waitForWindowClose(aWin, aCallback) {
  let winCount = JSON.parse(ss.getBrowserState()).windows.length;
  aWin.addEventListener("SSWindowClosing", function onWindowClosing() {
    aWin.removeEventListener("SSWindowClosing", onWindowClosing, false);
    function checkCount() {
      let state = JSON.parse(ss.getBrowserState());
      if (state.windows.length == (winCount - 1)) {
        aCallback();
      } else {
        executeSoon(checkCount);
      }
    }
    executeSoon(checkCount);
  }, false);
  aWin.close();
}

function forceWriteState(aCallback) {
  return promiseRecoveryFileContents().then(function(data) {
    aCallback(JSON.parse(data));
  });
}

function testOnWindow(aIsPrivate, aCallback) {
  whenNewWindowLoaded({private: aIsPrivate}, aCallback);
}

function waitForTabLoad(aWin, aURL, aCallback) {
  let browser = aWin.gBrowser.selectedBrowser;
  whenBrowserLoaded(browser, function () {
    TabState.flush(browser);
    executeSoon(aCallback);
  });
  browser.loadURI(aURL);
}
