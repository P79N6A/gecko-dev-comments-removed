


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

  
  let testPrivateBrowsing = function (aWindow) {
    aWindow.gBrowser.loadOneTab('http://mochi.test:8888/#1', {inBackground: true});
    aWindow.gBrowser.loadOneTab('http://mochi.test:8888/#2', {inBackground: true});

    let cw = getContentWindow(aWindow);
    let box = new cw.Rect(20, 20, 250, 200);
    let groupItem = new cw.GroupItem([], {bounds: box, immediately: true});
    cw.UI.setActive(groupItem);

    aWindow.gBrowser.selectedTab = aWindow.gBrowser.loadOneTab('http://mochi.test:8888/#3', {inBackground: true});
    aWindow.gBrowser.loadOneTab('http://mochi.test:8888/#4', {inBackground: true});

    afterAllTabsLoaded(function () {
      assertNumberOfVisibleTabs(aWindow, 2);

      enterAndLeavePrivateBrowsing(function () {
        assertNumberOfVisibleTabs(aWindow, 2);
        aWindow.gBrowser.selectedTab = aWindow.gBrowser.tabs[0];
        closeGroupItem(cw.GroupItems.groupItems[1], function() {
          next(aWindow);
        });
      });
    }, aWindow);
  }

  function testOnWindow(aIsPrivate, aCallback) {
    let win = OpenBrowserWindow({private: aIsPrivate});
    win.addEventListener("load", function onLoad() {
      win.removeEventListener("load", onLoad, false);
      executeSoon(function() { aCallback(win) });
    }, false);
  }

  function enterAndLeavePrivateBrowsing(callback) {
    testOnWindow(true, function (aWindow) {
      aWindow.close();
      callback();
    });
  }

  waitForExplicitFinish();

  
  tests.push(testUndoCloseTabs);

  
  tests.push(testDuplicateTab);
  tests.push(testBackForwardDuplicateTab);

  
  tests.push(testPrivateBrowsing);

  testOnWindow(false, function(aWindow) {
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