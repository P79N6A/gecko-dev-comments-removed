


function test() {
  let tests = [];

  let getContentWindow = function (aWindow) {
    return aWindow.TabView.getContentWindow();
  }

  let assertOneSingleGroupItem = function (aWindow) {
    is(getContentWindow(aWindow).GroupItems.groupItems.length, 1, 'There is one single groupItem');
  }

  let assertNumberOfVisibleTabs = function (aWindow, numTabs) {
    is(aWindow.gBrowser.visibleTabs.length, numTabs, 'There should be ' + numTabs + ' visible tabs');
  }

  let next = function (aWindow) {
    while (aWindow.gBrowser.tabs.length-1)
      aWindow.gBrowser.removeTab(aWindow.gBrowser.tabs[1]);

    hideTabView(function() {
      let callback = tests.shift();

      if (!callback) {
        executeSoon(function() {
          assertOneSingleGroupItem(aWindow);
          aWindow.close();
          finish();
        });
      } else {
        assertOneSingleGroupItem(aWindow);
        callback(aWindow);
      }
    }, aWindow);
  }

  
  let testUndoCloseTabs = function (aWindow) {
    aWindow.gBrowser.loadOneTab('http://mochi.test:8888/', {inBackground: true});
    aWindow.gBrowser.loadOneTab('http://mochi.test:8888/', {inBackground: true});

    afterAllTabsLoaded(function () {
      assertNumberOfVisibleTabs(aWindow, 3);

      aWindow.gBrowser.removeTab(aWindow.gBrowser.tabs[1]);
      aWindow.gBrowser.selectedTab = aWindow.gBrowser.tabs[1];

      restoreTab(function () {
        assertNumberOfVisibleTabs(aWindow, 3);
        assertOneSingleGroupItem(aWindow);
        next(aWindow);
      }, 0, aWindow);
    }, aWindow);
  }

  
  let testUndoCloseMultipleTabs = function (aWindow) {
    aWindow.gBrowser.loadOneTab('http://mochi.test:8888/', {inBackground: true});
    aWindow.gBrowser.loadOneTab('http://mochi.test:8888/', {inBackground: true});
    aWindow.gBrowser.loadOneTab('http://mochi.test:8888/', {inBackground: true});
    aWindow.gBrowser.loadOneTab('http://mochi.test:8888/', {inBackground: true});

    afterAllTabsLoaded(function () {
      assertNumberOfVisibleTabs(aWindow, 5);

      aWindow.gBrowser.removeTabsToTheEndFrom(aWindow.gBrowser.tabs[0]);
      assertNumberOfVisibleTabs(aWindow, 1);
      restoreTab(function () {
        assertNumberOfVisibleTabs(aWindow, 5);
        next(aWindow);
      }, undefined, aWindow);
    }, aWindow);
  }

  
  let testDuplicateTab = function (aWindow) {
    aWindow.gBrowser.loadOneTab('http://mochi.test:8888/', {inBackground: true});

    afterAllTabsLoaded(function () {
      
      aWindow.duplicateTabIn(aWindow.gBrowser.selectedTab, 'tab');

      afterAllTabsLoaded(function () {
        assertNumberOfVisibleTabs(aWindow, 3);
        assertOneSingleGroupItem(aWindow);
        next(aWindow);
      }, aWindow);
    }, aWindow);
  }

  
  let testBackForwardDuplicateTab = function (aWindow) {
    let tab = aWindow.gBrowser.loadOneTab('http://mochi.test:8888/#1', {inBackground: true});
    aWindow.gBrowser.selectedTab = tab;

    afterAllTabsLoaded(function () {
      tab.linkedBrowser.loadURI('http://mochi.test:8888/#2');

      afterAllTabsLoaded(function () {
        ok(aWindow.gBrowser.canGoBack, 'browser can go back in history');
        aWindow.BrowserBack({button: 1});

        afterAllTabsLoaded(function () {
          assertNumberOfVisibleTabs(aWindow, 3);

          ok(aWindow.gBrowser.canGoForward, 'browser can go forward in history');
          aWindow.BrowserForward({button: 1});

          afterAllTabsLoaded(function () {
            assertNumberOfVisibleTabs(aWindow, 4);
            assertOneSingleGroupItem(aWindow);
            next(aWindow);
          }, aWindow);
        }, aWindow);
      }, aWindow);
    }, aWindow);
  }

  function testOnWindow(aCallback) {
    let win = OpenBrowserWindow({private: false});
    win.addEventListener("load", function onLoad() {
      win.removeEventListener("load", onLoad, false);
      executeSoon(function() { aCallback(win) });
    }, false);
  }

  waitForExplicitFinish();

  
  tests.push(testUndoCloseTabs);

  
  tests.push(testUndoCloseMultipleTabs);

  
  tests.push(testDuplicateTab);
  tests.push(testBackForwardDuplicateTab);

  testOnWindow(function(aWindow) {
    loadTabView(function() {
      next(aWindow);
    }, aWindow);
  });
}

function loadTabView(callback, aWindow) {
  showTabView(function () {
    hideTabView(callback, aWindow);
  }, aWindow);
}
