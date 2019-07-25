




































const SS_SVC = Cc["@mozilla.org/browser/sessionstore;1"].
               getService(Ci.nsISessionStore);


function waitForBrowserState(aState, aSetStateCallback) {
  let windows = [window];
  let tabsRestored = 0;
  let expectedTabsRestored = 0;
  let expectedWindows = aState.windows.length;
  let windowsOpen = 1;

  aState.windows.forEach(function(winState) expectedTabsRestored += winState.tabs.length);

  function onSSTabRestored(aEvent) {
    if (++tabsRestored == expectedTabsRestored) {
      
      windows.forEach(function(win) {
        win.gBrowser.tabContainer.removeEventListener("SSTabRestored", onSSTabRestored, true);
      });
      info("running " + aSetStateCallback.name);
      executeSoon(aSetStateCallback);
    }
  }

  
  
  function windowObserver(aSubject, aTopic, aData) {
    if (aTopic == "domwindowopened") {
      let newWindow = aSubject.QueryInterface(Ci.nsIDOMWindow);
      newWindow.addEventListener("load", function() {
        newWindow.removeEventListener("load", arguments.callee, false);

        if (++windowsOpen == expectedWindows)
          Services.ww.unregisterNotification(windowObserver);

        
        windows.push(newWindow);
        
        newWindow.gBrowser.tabContainer.addEventListener("SSTabRestored", onSSTabRestored, true);
      }, false);
    }
  }

  
  if (expectedWindows > 1)
    Services.ww.registerNotification(windowObserver);

  
  gBrowser.tabContainer.addEventListener("SSTabRestored", onSSTabRestored, true);

  
  SS_SVC.setBrowserState(JSON.stringify(aState));
}



function waitForSaveState(aSaveStateCallback) {
  let topic = "sessionstore-state-write";
  Services.obs.addObserver(function() {
    Services.obs.removeObserver(arguments.callee, topic, false);
    executeSoon(aSaveStateCallback);
  }, topic, false);
};

var gUniqueCounter = 0;
function r() {
  return Date.now() + "-" + (++gUniqueCounter);
}
