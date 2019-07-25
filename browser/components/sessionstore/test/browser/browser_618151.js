




































Cu.import("resource://gre/modules/Services.jsm");
const ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);

const stateBackup = ss.getBrowserState();
const testState = {
  windows: [{
    tabs: [
      { entries: [{ url: "about:blank" }] },
      { entries: [{ url: "about:robots" }] }
    ]
  }]
};


function test() {
  
  waitForExplicitFinish();
  runNextTest();
}


let tests = [test_setup, test_hang];
function runNextTest() {
  
  if (tests.length) {
    
    
    var windowsEnum = Services.wm.getEnumerator("navigator:browser");
    while (windowsEnum.hasMoreElements()) {
      var currentWindow = windowsEnum.getNext();
      if (currentWindow != window) {
        currentWindow.close();
      }
    }

    let currentTest = tests.shift();
    info("running " + currentTest.name);
    waitForBrowserState(testState, currentTest);
  }
  else {
    ss.setBrowserState(stateBackup);
    executeSoon(finish);
  }
}


function waitForBrowserState(aState, aSetStateCallback) {
  let tabsRestored = 0;
  gBrowser.tabContainer.addEventListener("SSTabRestored", function() {
    if (++tabsRestored == aState.windows[0].tabs.length) {
      gBrowser.tabContainer.removeEventListener("SSTabRestored", arguments.callee, true);
      executeSoon(aSetStateCallback);
    }
  }, true);
  ss.setBrowserState(JSON.stringify(aState));
}


function test_setup() {
  function onSSTabRestored(aEvent) {
    gBrowser.tabContainer.removeEventListener("SSTabRestored", onSSTabRestored, false);
    runNextTest();
  }

  gBrowser.tabContainer.addEventListener("SSTabRestored", onSSTabRestored, false);
  ss.setTabState(gBrowser.tabs[1], JSON.stringify({
    entries: [{ url: "http://example.org" }],
    extData: { foo: "bar" } }));
}

function test_hang() {
  ok(true, "test didn't time out");
  runNextTest();
}

