




































const SS_SVC = Cc["@mozilla.org/browser/sessionstore;1"].
               getService(Ci.nsISessionStore);


function waitForBrowserState(aState, aSetStateCallback) {
  let windows = [window];
  let tabsRestored = 0;
  let expectedTabsRestored = 0;
  let expectedWindows = aState.windows.length;
  let windowsOpen = 1;
  let listening = false;
  let windowObserving = false;
  let restoreHiddenTabs = Services.prefs.getBoolPref(
                          "browser.sessionstore.restore_hidden_tabs");

  aState.windows.forEach(function (winState) {
    winState.tabs.forEach(function (tabState) {
      if (restoreHiddenTabs || !tabState.hidden)
        expectedTabsRestored++;
    });
  });

  
  
  if (!expectedTabsRestored)
    expectedTabsRestored = 1;

  function onSSTabRestored(aEvent) {
    if (++tabsRestored == expectedTabsRestored) {
      
      windows.forEach(function(win) {
        win.gBrowser.tabContainer.removeEventListener("SSTabRestored", onSSTabRestored, true);
      });
      listening = false;
      info("running " + aSetStateCallback.name);
      executeSoon(aSetStateCallback);
    }
  }

  
  
  function windowObserver(aSubject, aTopic, aData) {
    if (aTopic == "domwindowopened") {
      let newWindow = aSubject.QueryInterface(Ci.nsIDOMWindow);
      newWindow.addEventListener("load", function() {
        newWindow.removeEventListener("load", arguments.callee, false);

        if (++windowsOpen == expectedWindows) {
          Services.ww.unregisterNotification(windowObserver);
          windowObserving = false;
        }

        
        windows.push(newWindow);
        
        newWindow.gBrowser.tabContainer.addEventListener("SSTabRestored", onSSTabRestored, true);
      }, false);
    }
  }

  
  if (expectedWindows > 1) {
    registerCleanupFunction(function() {
      if (windowObserving) {
        Services.ww.unregisterNotification(windowObserver);
      }
    });
    windowObserving = true;
    Services.ww.registerNotification(windowObserver);
  }

  registerCleanupFunction(function() {
    if (listening) {
      windows.forEach(function(win) {
        win.gBrowser.tabContainer.removeEventListener("SSTabRestored", onSSTabRestored, true);
      });
    }
  });
  
  listening = true;
  gBrowser.tabContainer.addEventListener("SSTabRestored", onSSTabRestored, true);

  
  SS_SVC.setBrowserState(JSON.stringify(aState));
}



function waitForSaveState(aSaveStateCallback) {
  let observing = false;
  let topic = "sessionstore-state-write";

  let sessionSaveTimeout = 1000 +
    Services.prefs.getIntPref("browser.sessionstore.interval");

  let timeout = setTimeout(function () {
    Services.obs.removeObserver(observer, topic, false);
    aSaveStateCallback();
  }, sessionSaveTimeout);

  function observer(aSubject, aTopic, aData) {
    Services.obs.removeObserver(observer, topic, false);
    timeout = clearTimeout(timeout);
    observing = false;
    executeSoon(aSaveStateCallback);
  }

  registerCleanupFunction(function() {
    if (observing) {
      Services.obs.removeObserver(observer, topic, false);
    }
    if (timeout) {
      clearTimeout(timeout);
    }
  });

  observing = true;
  Services.obs.addObserver(observer, topic, false);
};

var gUniqueCounter = 0;
function r() {
  return Date.now() + "-" + (++gUniqueCounter);
}
