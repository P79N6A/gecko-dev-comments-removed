




































let stateBackup = ss.getBrowserState();

const TAB_STATE_NEEDS_RESTORE = 1;
const TAB_STATE_RESTORING = 2;

function test() {
  
  waitForExplicitFinish();
  
  
  requestLongerTimeout(4);
  runNextTest();
}




let tests = [test_cascade, test_select, test_multiWindowState,
             test_setWindowStateNoOverwrite, test_setWindowStateOverwrite,
             test_setBrowserStateInterrupted, test_reload,
              test_reloadCascadeSetup,
             ];
function runNextTest() {
  
  try {
    Services.prefs.clearUserPref("browser.sessionstore.max_concurrent_tabs");
  } catch (e) {}

  
  if (tests.length) {
    
    
    var windowsEnum = Services.wm.getEnumerator("navigator:browser");
    while (windowsEnum.hasMoreElements()) {
      var currentWindow = windowsEnum.getNext();
      if (currentWindow != window) {
        currentWindow.close();
      }
    }

    ss.setBrowserState(JSON.stringify({ windows: [{ tabs: [{ url: 'about:blank' }] }] }));
    let currentTest = tests.shift();
    info("running " + currentTest.name);
    executeSoon(currentTest);
  }
  else {
    ss.setBrowserState(stateBackup);
    executeSoon(finish);
  }
}


function test_cascade() {
  
  Services.prefs.setIntPref("browser.sessionstore.max_concurrent_tabs", 1);

  
  let progressListener = {
    onStateChange: function (aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
      dump("\n\nload: " + aBrowser.currentURI.spec + "\n" + JSON.stringify(countTabs()) + "\n\n");
      if (aBrowser.__SS_restoreState == TAB_STATE_RESTORING &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW)
        test_cascade_progressCallback();
    }
  }

  let state = { windows: [{ tabs: [
    { entries: [{ url: "http://example.com" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.com" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.com" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.com" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.com" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.com" }], extData: { "uniq": r() } }
  ] }] };

  let loadCount = 0;
  
  
  
  
  let expectedCounts = [
    [5, 1, 0],
    [4, 1, 1],
    [3, 1, 2],
    [2, 1, 3],
    [1, 1, 4],
    [0, 1, 5]
  ];

  function test_cascade_progressCallback() {
    loadCount++;
    let counts = countTabs();
    let expected = expectedCounts[loadCount - 1];

    is(counts[0], expected[0], "test_cascade: load " + loadCount + " - # tabs that need to be restored");
    is(counts[1], expected[1], "test_cascade: load " + loadCount + " - # tabs that are restoring");
    is(counts[2], expected[2], "test_cascade: load " + loadCount + " - # tabs that has been restored");

    if (loadCount < state.windows[0].tabs.length)
      return;

    window.gBrowser.removeTabsProgressListener(progressListener);
    runNextTest();
  }

  
  window.gBrowser.addTabsProgressListener(progressListener);
  ss.setBrowserState(JSON.stringify(state));
}


function test_select() {
  
  
  Services.prefs.setIntPref("browser.sessionstore.max_concurrent_tabs", 0);

  
  let progressListener = {
    onStateChange: function (aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
      if (aBrowser.__SS_restoreState == TAB_STATE_RESTORING &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW)
        test_select_progressCallback(aBrowser);
    }
  }

  let state = { windows: [{ tabs: [
    { entries: [{ url: "http://example.org" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org" }], extData: { "uniq": r() } }
  ], selected: 1 }] };

  let loadCount = 0;
  
  
  let expectedCounts = [
    [5, 1, 0],
    [4, 1, 1],
    [3, 1, 2],
    [2, 1, 3],
    [1, 1, 4],
    [0, 1, 5]
  ];
  let tabOrder = [0, 5, 1, 4, 3, 2];

  function test_select_progressCallback(aBrowser) {
    loadCount++;

    let counts = countTabs();
    let expected = expectedCounts[loadCount - 1];

    is(counts[0], expected[0], "test_select: load " + loadCount + " - # tabs that need to be restored");
    is(counts[1], expected[1], "test_select: load " + loadCount + " - # tabs that are restoring");
    is(counts[2], expected[2], "test_select: load " + loadCount + " - # tabs that has been restored");

    if (loadCount < state.windows[0].tabs.length) {
      
      let expectedData = state.windows[0].tabs[tabOrder[loadCount - 1]].extData.uniq;
      let tab;
      for (let i = 0; i < window.gBrowser.tabs.length; i++) {
        if (!tab && window.gBrowser.tabs[i].linkedBrowser == aBrowser)
          tab = window.gBrowser.tabs[i];
      }
      is(ss.getTabValue(tab, "uniq"), expectedData, "test_select: load " + loadCount + " - correct tab was restored");

      
      window.gBrowser.selectTabAtIndex(tabOrder[loadCount]);
      return;
    }

    window.gBrowser.removeTabsProgressListener(progressListener);
    runNextTest();
  }

  window.gBrowser.addTabsProgressListener(progressListener);
  ss.setBrowserState(JSON.stringify(state));
}


function test_multiWindowState() {
  
  let progressListener = {
    onStateChange: function (aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
      
      
      
      if (aBrowser.__SS_restoreState == TAB_STATE_RESTORING &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW)
        test_multiWindowState_progressCallback(aBrowser);
    }
  }

  
  
  let state = { windows: [
    {
      tabs: [
        { entries: [{ url: "http://example.org#0" }], extData: { "uniq": r() } }
      ],
      selected: 1
    },
    {
      tabs: [
        { entries: [{ url: "http://example.com#1" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#2" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#3" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#4" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#5" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#6" }], extData: { "uniq": r() } }
      ],
      selected: 4
    }
  ] };
  let numTabs = state.windows[0].tabs.length + state.windows[1].tabs.length;

  let loadCount = 0;
  function test_multiWindowState_progressCallback(aBrowser) {
    loadCount++;

    if (loadCount < numTabs)
      return;

    
    
    is(loadCount, numTabs, "test_multiWindowState: all tabs were restored");
    let count = countTabs();
    is(count[0], 0,
       "test_multiWindowState: there are no tabs left needing restore");

    
    
    window.gBrowser.removeTabsProgressListener(progressListener);
    runNextTest();
  }

  
  function windowObserver(aSubject, aTopic, aData) {
    let theWin = aSubject.QueryInterface(Ci.nsIDOMWindow);
    if (aTopic == "domwindowopened") {
      theWin.addEventListener("load", function() {
        theWin.removeEventListener("load", arguments.callee, false);

        Services.ww.unregisterNotification(windowObserver);
        theWin.gBrowser.addTabsProgressListener(progressListener);
      }, false);
    }
  }
  Services.ww.registerNotification(windowObserver);

  window.gBrowser.addTabsProgressListener(progressListener);
  ss.setBrowserState(JSON.stringify(state));
}


function test_setWindowStateNoOverwrite() {
  
  Services.prefs.setIntPref("browser.sessionstore.max_concurrent_tabs", 1);

  
  let progressListener = {
    onStateChange: function (aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
      
      
      
      if (aBrowser.__SS_restoreState == TAB_STATE_RESTORING &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW)
        test_setWindowStateNoOverwrite_progressCallback(aBrowser);
    }
  }

  
  
  let state1 = { windows: [{ tabs: [
    { entries: [{ url: "http://example.com#1" }] },
    { entries: [{ url: "http://example.com#2" }] },
    { entries: [{ url: "http://example.com#3" }] },
    { entries: [{ url: "http://example.com#4" }] },
    { entries: [{ url: "http://example.com#5" }] },
  ] }] };
  let state2 = { windows: [{ tabs: [
    { entries: [{ url: "http://example.org#1" }] },
    { entries: [{ url: "http://example.org#2" }] },
    { entries: [{ url: "http://example.org#3" }] },
    { entries: [{ url: "http://example.org#4" }] },
    { entries: [{ url: "http://example.org#5" }] }
  ] }] };

  let numTabs = state1.windows[0].tabs.length + state2.windows[0].tabs.length;

  let loadCount = 0;
  function test_setWindowStateNoOverwrite_progressCallback(aBrowser) {
    loadCount++;

    
    if (loadCount == 2)
      ss.setWindowState(window, JSON.stringify(state2), false);

    if (loadCount < numTabs)
      return;

    
    
    is(loadCount, numTabs, "test_setWindowStateNoOverwrite: all tabs were restored");
    
    
    
    is(window.__SS_tabsToRestore, 1,
       "test_setWindowStateNoOverwrite: window doesn't think there are more tabs to restore");
    let count = countTabs();
    is(count[0], 0,
       "test_setWindowStateNoOverwrite: there are no tabs left needing restore");

    
    
    window.gBrowser.removeTabsProgressListener(progressListener);

    runNextTest();
  }

  window.gBrowser.addTabsProgressListener(progressListener);
  ss.setWindowState(window, JSON.stringify(state1), true);
}


function test_setWindowStateOverwrite() {
  
  Services.prefs.setIntPref("browser.sessionstore.max_concurrent_tabs", 1);

  
  let progressListener = {
    onStateChange: function (aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
      
      
      
      if (aBrowser.__SS_restoreState == TAB_STATE_RESTORING &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW)
        test_setWindowStateOverwrite_progressCallback(aBrowser);
    }
  }

  
  
  let state1 = { windows: [{ tabs: [
    { entries: [{ url: "http://example.com#1" }] },
    { entries: [{ url: "http://example.com#2" }] },
    { entries: [{ url: "http://example.com#3" }] },
    { entries: [{ url: "http://example.com#4" }] },
    { entries: [{ url: "http://example.com#5" }] },
  ] }] };
  let state2 = { windows: [{ tabs: [
    { entries: [{ url: "http://example.org#1" }] },
    { entries: [{ url: "http://example.org#2" }] },
    { entries: [{ url: "http://example.org#3" }] },
    { entries: [{ url: "http://example.org#4" }] },
    { entries: [{ url: "http://example.org#5" }] }
  ] }] };

  let numTabs = 2 + state2.windows[0].tabs.length;

  let loadCount = 0;
  function test_setWindowStateOverwrite_progressCallback(aBrowser) {
    loadCount++;

    
    if (loadCount == 2)
      ss.setWindowState(window, JSON.stringify(state2), true);

    if (loadCount < numTabs)
      return;

    
    
    is(loadCount, numTabs, "test_setWindowStateOverwrite: all tabs were restored");
    
    
    
    is(window.__SS_tabsToRestore, 1,
       "test_setWindowStateOverwrite: window doesn't think there are more tabs to restore");
    let count = countTabs();
    is(count[0], 0,
       "test_setWindowStateOverwrite: there are no tabs left needing restore");

    
    
    window.gBrowser.removeTabsProgressListener(progressListener);

    runNextTest();
  }

  window.gBrowser.addTabsProgressListener(progressListener);
  ss.setWindowState(window, JSON.stringify(state1), true);
}


function test_setBrowserStateInterrupted() {
  
  Services.prefs.setIntPref("browser.sessionstore.max_concurrent_tabs", 1);

  
  let progressListener = {
    onStateChange: function (aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
      
      
      
      if (aBrowser.__SS_restoreState == TAB_STATE_RESTORING &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW)
        test_setBrowserStateInterrupted_progressCallback(aBrowser);
    }
  }

  
  
  let state1 = { windows: [
    {
      tabs: [
        { entries: [{ url: "http://example.org#1" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.org#2" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.org#3" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.org#4" }], extData: { "uniq": r() } }
      ],
      selected: 1
    },
    {
      tabs: [
        { entries: [{ url: "http://example.com#1" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#2" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#3" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#4" }], extData: { "uniq": r() } },
      ],
      selected: 3
    }
  ] };
  let state2 = { windows: [
    {
      tabs: [
        { entries: [{ url: "http://example.org#5" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.org#6" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.org#7" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.org#8" }], extData: { "uniq": r() } }
      ],
      selected: 3
    },
    {
      tabs: [
        { entries: [{ url: "http://example.com#5" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#6" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#7" }], extData: { "uniq": r() } },
        { entries: [{ url: "http://example.com#8" }], extData: { "uniq": r() } },
      ],
      selected: 1
    }
  ] };

  
  let interruptedAfter = 0;
  let loadedWindow1 = false;
  let loadedWindow2 = false;
  let numTabs = state2.windows[0].tabs.length + state2.windows[1].tabs.length;

  let loadCount = 0;
  function test_setBrowserStateInterrupted_progressCallback(aBrowser) {
    loadCount++;

    if (aBrowser.currentURI.spec == state1.windows[0].tabs[2].entries[0].url)
      loadedWindow1 = true;
    if (aBrowser.currentURI.spec == state1.windows[1].tabs[0].entries[0].url)
      loadedWindow2 = true;

    if (!interruptedAfter && loadedWindow1 && loadedWindow2) {
      interruptedAfter = loadCount;
      ss.setBrowserState(JSON.stringify(state2));
      return;
    }

    if (loadCount < numTabs + interruptedAfter)
      return;

    
    
    is(loadCount, numTabs + interruptedAfter,
       "test_setBrowserStateInterrupted: all tabs were restored");
    let count = countTabs();
    is(count[0], 0,
       "test_setBrowserStateInterrupted: there are no tabs left needing restore");

    
    
    window.gBrowser.removeTabsProgressListener(progressListener);
    Services.ww.unregisterNotification(windowObserver);
    runNextTest();
  }

  
  function windowObserver(aSubject, aTopic, aData) {
    let theWin = aSubject.QueryInterface(Ci.nsIDOMWindow);
    if (aTopic == "domwindowopened") {
      theWin.addEventListener("load", function() {
        theWin.removeEventListener("load", arguments.callee, false);

        Services.ww.unregisterNotification(windowObserver);
        theWin.gBrowser.addTabsProgressListener(progressListener);
      }, false);
    }
  }
  Services.ww.registerNotification(windowObserver);

  window.gBrowser.addTabsProgressListener(progressListener);
  ss.setBrowserState(JSON.stringify(state1));
}


function test_reload() {
  
  
  Services.prefs.setIntPref("browser.sessionstore.max_concurrent_tabs", 0);

  
  let progressListener = {
    onStateChange: function (aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
      if (aBrowser.__SS_restoreState == TAB_STATE_RESTORING &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW)
        test_reload_progressCallback(aBrowser);
    }
  }

  let state = { windows: [{ tabs: [
    { entries: [{ url: "http://example.org/#1" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#2" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#3" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#4" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#5" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#6" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#7" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#8" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#9" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#10" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#11" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#12" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#13" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#14" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#15" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#16" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#17" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.org/#18" }], extData: { "uniq": r() } }
  ], selected: 1 }] };

  let loadCount = 0;
  function test_reload_progressCallback(aBrowser) {
    loadCount++;

    is(aBrowser.currentURI.spec, state.windows[0].tabs[loadCount - 1].entries[0].url,
       "test_reload: load " + loadCount + " - browser loaded correct url");

    if (loadCount <= state.windows[0].tabs.length) {
      
      let expectedData = state.windows[0].tabs[loadCount - 1].extData.uniq;
      let tab;
      for (let i = 0; i < window.gBrowser.tabs.length; i++) {
        if (!tab && window.gBrowser.tabs[i].linkedBrowser == aBrowser)
          tab = window.gBrowser.tabs[i];
      }
      is(ss.getTabValue(tab, "uniq"), expectedData,
         "test_reload: load " + loadCount + " - correct tab was restored");

      if (loadCount == state.windows[0].tabs.length) {
        window.gBrowser.removeTabsProgressListener(progressListener);
        executeSoon(function() {
          _test_reloadAfter("test_reloadReload", state, runNextTest);
        });
      }
      else {
        
        window.gBrowser.reloadTab(window.gBrowser.tabs[loadCount]);
      }
    }

  }

  window.gBrowser.addTabsProgressListener(progressListener);
  ss.setBrowserState(JSON.stringify(state));
}




function test_reloadCascadeSetup() {
  
  let progressListener = {
    onStateChange: function (aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
      if (aBrowser.__SS_restoreState == TAB_STATE_RESTORING &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW)
        test_cascadeReloadSetup_progressCallback();
    }
  }

  let state = { windows: [{ tabs: [
    { entries: [{ url: "http://example.com/#1" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.com/#2" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.com/#3" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.com/#4" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.com/#5" }], extData: { "uniq": r() } },
    { entries: [{ url: "http://example.com/#6" }], extData: { "uniq": r() } }
  ] }] };

  let loadCount = 0;
  function test_cascadeReloadSetup_progressCallback() {
    loadCount++;
    if (loadCount < state.windows[0].tabs.length)
      return;

    window.gBrowser.removeTabsProgressListener(progressListener);
    executeSoon(function() {
      _test_reloadAfter("test_reloadCascade", state, runNextTest);
    });
  }

  
  window.gBrowser.addTabsProgressListener(progressListener);
  ss.setBrowserState(JSON.stringify(state));
}






function _test_reloadAfter(aTestName, aState, aCallback) {
  info("starting " + aTestName);
  let progressListener = {
    onStateChange: function (aBrowser, aWebProgress, aRequest, aStateFlags, aStatus) {
      if (aStateFlags & Ci.nsIWebProgressListener.STATE_STOP &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_NETWORK &&
          aStateFlags & Ci.nsIWebProgressListener.STATE_IS_WINDOW)
        test_reloadAfter_progressCallback(aBrowser);
    }
  }

  
  
  let fakeEvent = {
    button: 0,
    metaKey: false,
    altKey: false,
    ctrlKey: false,
    shiftKey: false,
  }

  let loadCount = 0;
  function test_reloadAfter_progressCallback(aBrowser) {
    loadCount++;

    if (loadCount <= aState.windows[0].tabs.length) {
      
      let expectedData = aState.windows[0].tabs[loadCount - 1].extData.uniq;
      let tab;
      for (let i = 0; i < window.gBrowser.tabs.length; i++) {
        if (!tab && window.gBrowser.tabs[i].linkedBrowser == aBrowser)
          tab = window.gBrowser.tabs[i];
      }
      is(ss.getTabValue(tab, "uniq"), expectedData,
         aTestName + ": load " + loadCount + " - correct tab was reloaded");

      if (loadCount == aState.windows[0].tabs.length) {
        window.gBrowser.removeTabsProgressListener(progressListener);
        aCallback();
      }
      else {
        
        window.gBrowser.selectTabAtIndex(loadCount);
        BrowserReloadOrDuplicate(fakeEvent);
      }
    }
  }

  window.gBrowser.addTabsProgressListener(progressListener);
  BrowserReloadOrDuplicate(fakeEvent);
}


function countTabs() {
  let needsRestore = 0,
      isRestoring = 0,
      wasRestored = 0;

  let windowsEnum = Services.wm.getEnumerator("navigator:browser");

  while (windowsEnum.hasMoreElements()) {
    let window = windowsEnum.getNext();
    if (window.closed)
      continue;

    for (let i = 0; i < window.gBrowser.tabs.length; i++) {
      let browser = window.gBrowser.tabs[i].linkedBrowser;
      if (browser.__SS_restoreState == TAB_STATE_RESTORING)
        isRestoring++;
      else if (browser.__SS_restoreState == TAB_STATE_NEEDS_RESTORE)
        needsRestore++;
      else
        wasRestored++;
    }
  }
  return [needsRestore, isRestoring, wasRestored];
}

