



const TAB_STATE_NEEDS_RESTORE = 1;
const TAB_STATE_RESTORING = 2;

const FRAME_SCRIPT = "chrome://mochitests/content/browser/browser/components/" +
                     "sessionstore/test/content.js";

let mm = Cc["@mozilla.org/globalmessagemanager;1"]
           .getService(Ci.nsIMessageListenerManager);
mm.loadFrameScript(FRAME_SCRIPT, true);
mm.addMessageListener("SessionStore:setupSyncHandler", onSetupSyncHandler);






let SyncHandlers = new WeakMap();
function onSetupSyncHandler(msg) {
  SyncHandlers.set(msg.target, msg.objects.handler);
}

registerCleanupFunction(() => {
  mm.removeDelayedFrameScript(FRAME_SCRIPT);
  mm.removeMessageListener("SessionStore:setupSyncHandler", onSetupSyncHandler);
});

let tmp = {};
Cu.import("resource://gre/modules/Promise.jsm", tmp);
Cu.import("resource:///modules/sessionstore/SessionStore.jsm", tmp);
let {Promise, SessionStore} = tmp;

let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);




Services.prefs.setBoolPref("browser.sessionstore.restore_on_demand", false);
registerCleanupFunction(function () {
  Services.prefs.clearUserPref("browser.sessionstore.restore_on_demand");
});


Services.prefs.setBoolPref("browser.sessionstore.debug", true);
registerCleanupFunction(function () {
  Services.prefs.clearUserPref("browser.sessionstore.debug");
});




Cc["@mozilla.org/browser/clh;1"].getService(Ci.nsIBrowserHandler).defaultArgs;

function provideWindow(aCallback, aURL, aFeatures) {
  function callbackSoon(aWindow) {
    executeSoon(function executeCallbackSoon() {
      aCallback(aWindow);
    });
  }

  let win = openDialog(getBrowserURL(), "", aFeatures || "chrome,all,dialog=no", aURL);
  whenWindowLoaded(win, function onWindowLoaded(aWin) {
    if (!aURL) {
      info("Loaded a blank window.");
      callbackSoon(aWin);
      return;
    }

    aWin.gBrowser.selectedBrowser.addEventListener("load", function selectedBrowserLoadListener() {
      aWin.gBrowser.selectedBrowser.removeEventListener("load", selectedBrowserLoadListener, true);
      callbackSoon(aWin);
    }, true);
  });
}


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

  
  ss.setBrowserState(JSON.stringify(aState));
}



function waitForTabState(aTab, aState, aCallback) {
  let listening = true;

  function onSSTabRestored() {
    aTab.removeEventListener("SSTabRestored", onSSTabRestored, false);
    listening = false;
    aCallback();
  }

  aTab.addEventListener("SSTabRestored", onSSTabRestored, false);

  registerCleanupFunction(function() {
    if (listening) {
      aTab.removeEventListener("SSTabRestored", onSSTabRestored, false);
    }
  });
  ss.setTabState(aTab, JSON.stringify(aState));
}




function promiseContentMessage(browser, name) {
  let deferred = Promise.defer();
  let mm = browser.messageManager;

  function removeListener() {
    mm.removeMessageListener(name, listener);
  }

  function listener(msg) {
    removeListener();
    deferred.resolve(msg.data);
  }

  mm.addMessageListener(name, listener);
  registerCleanupFunction(removeListener);
  return deferred.promise;
}

function waitForTopic(aTopic, aTimeout, aCallback) {
  let observing = false;
  function removeObserver() {
    if (!observing)
      return;
    Services.obs.removeObserver(observer, aTopic);
    observing = false;
  }

  let timeout = setTimeout(function () {
    removeObserver();
    aCallback(false);
  }, aTimeout);

  function observer(aSubject, aTopic, aData) {
    removeObserver();
    timeout = clearTimeout(timeout);
    executeSoon(() => aCallback(true));
  }

  registerCleanupFunction(function() {
    removeObserver();
    if (timeout) {
      clearTimeout(timeout);
    }
  });

  observing = true;
  Services.obs.addObserver(observer, aTopic, false);
}














function waitForSaveState(aCallback) {
  let timeout = 100 +
    Services.prefs.getIntPref("browser.sessionstore.interval");
  return waitForTopic("sessionstore-state-write", timeout, aCallback);
}
function promiseSaveState() {
  let deferred = Promise.defer();
  waitForSaveState(isSuccessful => {
    if (isSuccessful) {
      deferred.resolve();
    } else {
      deferred.reject(new Error("timeout"));
    }});
  return deferred.promise;
}
function forceSaveState() {
  let promise = promiseSaveState();
  const PREF = "browser.sessionstore.interval";
  
  
  Services.prefs.setIntPref(PREF, 1000);
  Services.prefs.setIntPref(PREF, 0);
  return promise.then(
    function onSuccess(x) {
      Services.prefs.clearUserPref(PREF);
      return x;
    },
    function onError(x) {
      Services.prefs.clearUserPref(PREF);
      throw x;
    }
  );
}

function whenBrowserLoaded(aBrowser, aCallback = next) {
  aBrowser.addEventListener("load", function onLoad(event) {
    if (event.target == aBrowser.contentDocument) {
      aBrowser.removeEventListener("load", onLoad, true);
      executeSoon(aCallback);
    }
  }, true);
}
function promiseBrowserLoaded(aBrowser) {
  let deferred = Promise.defer();
  whenBrowserLoaded(aBrowser, deferred.resolve);
  return deferred.promise;
}
function whenBrowserUnloaded(aBrowser, aContainer, aCallback = next) {
  aBrowser.addEventListener("unload", function onUnload() {
    aBrowser.removeEventListener("unload", onUnload, true);
    executeSoon(aCallback);
  }, true);
}
function promiseBrowserUnloaded(aBrowser, aContainer) {
  let deferred = Promise.defer();
  whenBrowserUnloaded(aBrowser, aContainer, deferred.resolve);
  return deferred.promise;
}

function whenWindowLoaded(aWindow, aCallback = next) {
  aWindow.addEventListener("load", function windowLoadListener() {
    aWindow.removeEventListener("load", windowLoadListener, false);
    executeSoon(function executeWhenWindowLoaded() {
      aCallback(aWindow);
    });
  }, false);
}

function whenTabRestored(aTab, aCallback = next) {
  aTab.addEventListener("SSTabRestored", function onRestored(aEvent) {
    aTab.removeEventListener("SSTabRestored", onRestored, true);
    executeSoon(function executeWhenTabRestored() {
      aCallback();
    });
  }, true);
}

var gUniqueCounter = 0;
function r() {
  return Date.now() + "-" + (++gUniqueCounter);
}

function BrowserWindowIterator() {
  let windowsEnum = Services.wm.getEnumerator("navigator:browser");
  while (windowsEnum.hasMoreElements()) {
    let currentWindow = windowsEnum.getNext();
    if (!currentWindow.closed) {
      yield currentWindow;
    }
  }
}

let gProgressListener = {
  _callback: null,
  _checkRestoreState: true,

  setCallback: function gProgressListener_setCallback(aCallback, aCheckRestoreState = true) {
    if (!this._callback) {
      window.gBrowser.addTabsProgressListener(this);
    }
    this._callback = aCallback;
    this._checkRestoreState = aCheckRestoreState;
  },

  unsetCallback: function gProgressListener_unsetCallback() {
    if (this._callback) {
      this._callback = null;
      window.gBrowser.removeTabsProgressListener(this);
    }
  },

  onStateChange:
  function gProgressListener_onStateChange(aBrowser, aWebProgress, aRequest,
                                           aStateFlags, aStatus) {
    if ((!this._checkRestoreState ||
         (aBrowser.__SS_restoreState && aBrowser.__SS_restoreState == TAB_STATE_RESTORING)) &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW) {
      let args = [aBrowser].concat(this._countTabs());
      this._callback.apply(this, args);
    }
  },

  _countTabs: function gProgressListener_countTabs() {
    let needsRestore = 0, isRestoring = 0, wasRestored = 0;

    for (let win in BrowserWindowIterator()) {
      for (let i = 0; i < win.gBrowser.tabs.length; i++) {
        let browser = win.gBrowser.tabs[i].linkedBrowser;
        if (!browser.__SS_restoreState)
          wasRestored++;
        else if (browser.__SS_restoreState == TAB_STATE_RESTORING)
          isRestoring++;
        else if (browser.__SS_restoreState == TAB_STATE_NEEDS_RESTORE)
          needsRestore++;
      }
    }
    return [needsRestore, isRestoring, wasRestored];
  }
};

registerCleanupFunction(function () {
  gProgressListener.unsetCallback();
});



function closeAllButPrimaryWindow() {
  for (let win in BrowserWindowIterator()) {
    if (win != window) {
      win.close();
    }
  }
}









function whenNewWindowLoaded(aOptions, aCallback) {
  let win = OpenBrowserWindow(aOptions);
  whenDelayedStartupFinished(win, () => aCallback(win));
  return win;
}
function promiseNewWindowLoaded(aOptions) {
  let deferred = Promise.defer();
  whenNewWindowLoaded(aOptions, deferred.resolve);
  return deferred.promise;
}





function promiseWindowClosed(win) {
  let deferred = Promise.defer();

  Services.obs.addObserver(function obs(subject, topic) {
    if (subject == win) {
      Services.obs.removeObserver(obs, topic);
      deferred.resolve();
    }
  }, "domwindowclosed", false);

  win.close();
  return deferred.promise;
}






function whenDelayedStartupFinished(aWindow, aCallback) {
  Services.obs.addObserver(function observer(aSubject, aTopic) {
    if (aWindow == aSubject) {
      Services.obs.removeObserver(observer, aTopic);
      executeSoon(aCallback);
    }
  }, "browser-delayed-startup-finished", false);
}




let TestRunner = {
  _iter: null,

  



  backupState: {},

  


  run: function () {
    waitForExplicitFinish();

    SessionStore.promiseInitialized.then(() => {
      this.backupState = JSON.parse(ss.getBrowserState());
      this._iter = runTests();
      this.next();
    });
  },

  


  next: function () {
    try {
      TestRunner._iter.next();
    } catch (e if e instanceof StopIteration) {
      TestRunner.finish();
    }
  },

  


  finish: function () {
    closeAllButPrimaryWindow();
    waitForBrowserState(this.backupState, finish);
  }
};

function next() {
  TestRunner.next();
}

function promiseTabRestored(tab) {
  let deferred = Promise.defer();

  tab.addEventListener("SSTabRestored", function onRestored() {
    tab.removeEventListener("SSTabRestored", onRestored);
    deferred.resolve();
  });

  return deferred.promise;
}

function sendMessage(browser, name, data = {}) {
  browser.messageManager.sendAsyncMessage(name, data);
  return promiseContentMessage(browser, name);
}
