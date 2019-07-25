





































const WINDOW_ATTRIBUTES = ["width", "height", "screenX", "screenY", "sizemode"];

let ss = Cc["@mozilla.org/browser/sessionstore;1"].
         getService(Ci.nsISessionStore);

let stateBackup = ss.getBrowserState();

let originalWarnOnClose = gPrefService.getBoolPref("browser.tabs.warnOnClose");
let originalStartupPage = gPrefService.getIntPref("browser.startup.page");
let originalWindowType = document.documentElement.getAttribute("windowtype");

let gotLastWindowClosedTopic = false;
let shouldPinTab = false;
let shouldOpenTabs = false;
let shouldCloseTab = false;
let testNum = 0;
let afterTestCallback;


let testState = {
  windows: [
    { tabs: [{ entries: [{ url: "http://example.org" }] }] }
  ],
  _closedWindows: []
};




let tests = [];


function checkOSX34Generator(num) {
  return function(aPreviousState, aCurState) {
    
    
    let expectedState = JSON.parse(aPreviousState);
    expectedState[0].tabs.shift();
    
    
    WINDOW_ATTRIBUTES.forEach(function (attr) delete expectedState[0][attr]);

    is(aCurState, JSON.stringify(expectedState),
       "test #" + num + ": closedWindowState is as expected");
  };
}
function checkNoWindowsGenerator(num) {
  return function(aPreviousState, aCurState) {
    is(aCurState, "[]", "test #" + num + ": there should be no closedWindowsLeft");
  };
}


tests.push({
  pinned: false,
  extra: false,
  close: false,
  checkWinLin: checkNoWindowsGenerator(1),
  checkOSX: function(aPreviousState, aCurState) {
    is(aCurState, aPreviousState, "test #1: closed window state is unchanged");
  }
});


tests.push({
  pinned: true,
  extra: false,
  close: false,
  checkWinLin: checkNoWindowsGenerator(2),
  checkOSX: checkNoWindowsGenerator(2)
});


tests.push({
  pinned: true,
  extra: true,
  close: false,
  checkWinLin: checkNoWindowsGenerator(3),
  checkOSX: checkOSX34Generator(3)
});


tests.push({
  pinned: true,
  extra: true,
  close: "one",
  checkWinLin: checkNoWindowsGenerator(4),
  checkOSX: checkOSX34Generator(4)
});


tests.push({
  pinned: true,
  extra: true,
  close: "both",
  checkWinLin: checkNoWindowsGenerator(5),
  checkOSX: checkNoWindowsGenerator(5)
});


function test() {
  

  waitForExplicitFinish();
  
  requestLongerTimeout(2);

  
  gPrefService.setBoolPref("browser.tabs.warnOnClose", false);
  
  gPrefService.setIntPref("browser.startup.page", 3);

  runNextTestOrFinish();
}

function runNextTestOrFinish() {
  if (tests.length) {
    setupForTest(tests.shift())
  }
  else {
    
    ["browser.tabs.warnOnClose", "browser.startup.page"].forEach(function(p) {
      if (gPrefService.prefHasUserValue(p))
        gPrefService.clearUserPref(p);
    });

    ss.setBrowserState(stateBackup);
    executeSoon(finish);
  }
}

function setupForTest(aConditions) {
  
  gotLastWindowClosedTopic = false;
  shouldPinTab = aConditions.pinned;
  shouldOpenTabs = aConditions.extra;
  shouldCloseTab = aConditions.close;
  testNum++;

  
  afterTestCallback = /Mac/.test(navigator.platform) ? aConditions.checkOSX
                                                     : aConditions.checkWinLin;

  
  Services.obs.addObserver(onLastWindowClosed, "browser-lastwindow-close-granted", false);

  
  Services.obs.addObserver(onStateRestored, "sessionstore-browser-state-restored", false);
  ss.setBrowserState(JSON.stringify(testState));
}

function onStateRestored(aSubject, aTopic, aData) {
  info("test #" + testNum + ": onStateRestored");
  Services.obs.removeObserver(onStateRestored, "sessionstore-browser-state-restored", false);

  
  
  document.documentElement.setAttribute("windowtype", "navigator:testrunner");

  let newWin = openDialog(location, "_blank", "chrome,all,dialog=no", "http://example.com");
  newWin.addEventListener("load", function(aEvent) {
    newWin.removeEventListener("load", arguments.callee, false);

    newWin.gBrowser.selectedBrowser.addEventListener("load", function() {
      newWin.gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

      
      if (shouldPinTab)
        newWin.gBrowser.pinTab(newWin.gBrowser.selectedTab);

      newWin.addEventListener("unload", function () {
        newWin.removeEventListener("unload", arguments.callee, false);
        onWindowUnloaded();
      }, false);
      
      
      
      if (shouldOpenTabs) {
        let newTab = newWin.gBrowser.addTab("about:config");
        let newTab2 = newWin.gBrowser.addTab("about:buildconfig");

        newTab.linkedBrowser.addEventListener("load", function() {
          newTab.linkedBrowser.removeEventListener("load", arguments.callee, true);

          if (shouldCloseTab == "one") {
            newWin.gBrowser.removeTab(newTab2);
          }
          else if (shouldCloseTab == "both") {
            newWin.gBrowser.removeTab(newTab);
            newWin.gBrowser.removeTab(newTab2);
          }
          newWin.BrowserTryToCloseWindow();
        }, true);
      }
      else {
        newWin.BrowserTryToCloseWindow();
      }
    }, true);
  }, false);
}


function onLastWindowClosed(aSubject, aTopic, aData) {
  info("test #" + testNum + ": onLastWindowClosed");
  Services.obs.removeObserver(onLastWindowClosed, "browser-lastwindow-close-granted", false);
  gotLastWindowClosedTopic = true;
}





function onWindowUnloaded() {
  info("test #" + testNum + ": onWindowClosed");
  ok(gotLastWindowClosedTopic, "test #" + testNum + ": browser-lastwindow-close-granted was notified prior");

  let previousClosedWindowData = ss.getClosedWindowData();

  
  let newWin = openDialog(location, "_blank", "chrome,all,dialog=no", "about:robots");
  newWin.addEventListener("load", function(aEvent) {
    newWin.removeEventListener("load", arguments.callee, false);

    newWin.gBrowser.selectedBrowser.addEventListener("load", function () {
      newWin.gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);

      
      afterTestCallback(previousClosedWindowData, ss.getClosedWindowData());
      afterTestCleanup(newWin);
    }, true);

  }, false);
}

function afterTestCleanup(aNewWin) {
  executeSoon(function() {
    aNewWin.close();
    document.documentElement.setAttribute("windowtype", originalWindowType);
    runNextTestOrFinish();
  });
}
