



const TAB_STATE_NEEDS_RESTORE = 1;
const TAB_STATE_RESTORING = 2;

const ROOT = getRootDirectory(gTestPath);
const FRAME_SCRIPTS = [
  ROOT + "content.js",
  ROOT + "content-forms.js"
];

let mm = Cc["@mozilla.org/globalmessagemanager;1"]
           .getService(Ci.nsIMessageListenerManager);

for (let script of FRAME_SCRIPTS) {
  mm.loadFrameScript(script, true);
}

mm.addMessageListener("SessionStore:setupSyncHandler", onSetupSyncHandler);






let SyncHandlers = new WeakMap();
function onSetupSyncHandler(msg) {
  SyncHandlers.set(msg.target, msg.objects.handler);
}

registerCleanupFunction(() => {
  for (let script of FRAME_SCRIPTS) {
    mm.removeDelayedFrameScript(script, true);
  }
  mm.removeMessageListener("SessionStore:setupSyncHandler", onSetupSyncHandler);
});

let tmp = {};
Cu.import("resource://gre/modules/Promise.jsm", tmp);
Cu.import("resource://gre/modules/Task.jsm", tmp);
Cu.import("resource:///modules/sessionstore/SessionStore.jsm", tmp);
Cu.import("resource:///modules/sessionstore/SessionSaver.jsm", tmp);
Cu.import("resource:///modules/sessionstore/SessionFile.jsm", tmp);
let {Promise, Task, SessionStore, SessionSaver, SessionFile} = tmp;

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

  let win = openDialog(getBrowserURL(), "", aFeatures || "chrome,all,dialog=no", aURL || "about:blank");
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
  if (typeof aState == "string") {
    aState = JSON.parse(aState);
  }
  if (typeof aState != "object") {
    throw new TypeError("Argument must be an object or a JSON representation of an object");
  }
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

  
  gBrowser.selectedTab = gBrowser.tabs[0];

  
  ss.setBrowserState(JSON.stringify(aState));
}

function promiseBrowserState(aState) {
  return new Promise(resolve => waitForBrowserState(aState, resolve));
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
  return waitForTopic("sessionstore-state-write-complete", timeout, aCallback);
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
  return SessionSaver.run();
}

function promiseRecoveryFileContents() {
  let promise = forceSaveState();
  return promise.then(function() {
    return OS.File.read(SessionFile.Paths.recovery, { encoding: "utf-8" });
  });
}

let promiseForEachSessionRestoreFile = Task.async(function*(cb) {
  for (let key of SessionFile.Paths.loadOrder) {
    let data = "";
    try {
      data = yield OS.File.read(SessionFile.Paths[key], { encoding: "utf-8" });
    } catch (ex if ex instanceof OS.File.Error
	     && ex.becauseNoSuchFile) {
      
    }
    cb(data, key);
  }
});

function whenBrowserLoaded(aBrowser, aCallback = next, ignoreSubFrames = true, expectedURL = null) {
  aBrowser.messageManager.addMessageListener("ss-test:loadEvent", function onLoad(msg) {
    if (expectedURL && aBrowser.currentURI.spec != expectedURL) {
      return;
    }

    if (!ignoreSubFrames || !msg.data.subframe) {
      aBrowser.messageManager.removeMessageListener("ss-test:loadEvent", onLoad);
      executeSoon(aCallback);
    }
  });
}
function promiseBrowserLoaded(aBrowser, ignoreSubFrames = true) {
  let deferred = Promise.defer();
  whenBrowserLoaded(aBrowser, deferred.resolve, ignoreSubFrames);
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
function promiseWindowLoaded(aWindow) {
  let deferred = Promise.defer();
  whenWindowLoaded(aWindow, deferred.resolve);
  return deferred.promise;
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

let gWebProgressListener = {
  _callback: null,

  setCallback: function (aCallback) {
    if (!this._callback) {
      window.gBrowser.addTabsProgressListener(this);
    }
    this._callback = aCallback;
  },

  unsetCallback: function () {
    if (this._callback) {
      this._callback = null;
      window.gBrowser.removeTabsProgressListener(this);
    }
  },

  onStateChange: function (aBrowser, aWebProgress, aRequest,
                           aStateFlags, aStatus) {
    if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
        aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW) {
      this._callback(aBrowser);
    }
  }
};

registerCleanupFunction(function () {
  gWebProgressListener.unsetCallback();
});

let gProgressListener = {
  _callback: null,

  setCallback: function (callback) {
    Services.obs.addObserver(this, "sessionstore-debug-tab-restored", false);
    this._callback = callback;
  },

  unsetCallback: function () {
    if (this._callback) {
      this._callback = null;
    Services.obs.removeObserver(this, "sessionstore-debug-tab-restored");
    }
  },

  observe: function (browser, topic, data) {
    gProgressListener.onRestored(browser);
  },

  onRestored: function (browser) {
    if (browser.__SS_restoreState == TAB_STATE_RESTORING) {
      let args = [browser].concat(gProgressListener._countTabs());
      gProgressListener._callback.apply(gProgressListener, args);
    }
  },

  _countTabs: function () {
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
  let features = "";
  let url = "about:blank";

  if (aOptions && aOptions.private || false) {
    features = ",private";
    url = "about:privatebrowsing";
  }

  let win = openDialog(getBrowserURL(), "", "chrome,all,dialog=no" + features, url);
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

function runInContent(browser, func, arg, callback = null) {
  let deferred = Promise.defer();

  let mm = browser.messageManager;
  mm.sendAsyncMessage("ss-test:run", {code: func.toSource()}, {arg: arg});
  mm.addMessageListener("ss-test:runFinished", ({data}) => deferred.resolve(data));

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
function promiseDelayedStartupFinished(aWindow) {
  return new Promise((resolve) => whenDelayedStartupFinished(aWindow, resolve));
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
    gBrowser.selectedTab = gBrowser.tabs[0];
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




const FORM_HELPERS = [
  "getTextContent",
  "getInputValue", "setInputValue",
  "getInputChecked", "setInputChecked",
  "getSelectedIndex", "setSelectedIndex",
  "getMultipleSelected", "setMultipleSelected",
  "getFileNameArray", "setFileNameArray",
];

for (let name of FORM_HELPERS) {
  let msg = "ss-test:" + name;
  this[name] = (browser, data) => sendMessage(browser, msg, data);
}
