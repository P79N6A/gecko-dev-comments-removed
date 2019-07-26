



function test() {
  waitForExplicitFinish();

  
  Services.prefs.setIntPref(phishyUserPassPref, 32);
  registerCleanupFunction(function () {
    Services.prefs.clearUserPref(phishyUserPassPref);
  });

  nextTest();
}

const phishyUserPassPref = "network.http.phishy-userpass-length";

function nextTest() {
  let test = tests.shift();
  if (test) {
    test(function () {
      executeSoon(nextTest);
    });
  } else {
    executeSoon(finish);
  }
}

let tests = [
  function revert(next) {
    loadTabInWindow(window, function (tab) {
      gURLBar.handleRevert();
      is(gURLBar.value, "example.com", "URL bar had user/pass stripped after reverting");
      gBrowser.removeTab(tab);
      next();
    });
  },
  function customize(next) {
    whenNewWindowLoaded(undefined, function (win) {
      
      
      whenDelayedStartupFinished(win, function () {
        loadTabInWindow(win, function () {
          openToolbarCustomizationUI(function () {
            closeToolbarCustomizationUI(function () {
              is(win.gURLBar.value, "example.com", "URL bar had user/pass stripped after customize");
              win.close();
              next();
            }, win);
          }, win);
        });
      });
    });
  },
  function pageloaderror(next) {
    loadTabInWindow(window, function (tab) {
      
      
      tab.linkedBrowser.loadURI("http://test1.example.com");
      tab.linkedBrowser.stop();
      is(gURLBar.value, "example.com", "URL bar had user/pass stripped after load error");
      gBrowser.removeTab(tab);
      next();
    });
  }
];

function loadTabInWindow(win, callback) {
  info("Loading tab");
  let url = "http://user:pass@example.com/";
  let tab = win.gBrowser.selectedTab = win.gBrowser.addTab(url);
  tab.linkedBrowser.addEventListener("load", function listener() {
    info("Tab loaded");
    if (tab.linkedBrowser.currentURI.spec != url)
      return;
    tab.linkedBrowser.removeEventListener("load", listener, true);

    is(win.gURLBar.value, "example.com", "URL bar had user/pass stripped initially");
    callback(tab);
  }, true);
}
