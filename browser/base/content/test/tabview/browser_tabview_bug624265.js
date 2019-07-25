


let pb = Cc['@mozilla.org/privatebrowsing;1'].
         getService(Ci.nsIPrivateBrowsingService);

function test() {
  let tests = [];

  let getContentWindow = function () {
    return TabView.getContentWindow();
  }

  let assertOneSingleGroupItem = function () {
    is(getContentWindow().GroupItems.groupItems.length, 1, 'There is one single groupItem');
  }

  let assertNumberOfVisibleTabs = function (numTabs) {
    is(gBrowser.visibleTabs.length, numTabs, 'There should be ' + numTabs + ' visible tabs');
  }

  let restoreTab = function (callback) {
    let tab = undoCloseTab(0);
    
    tab._tabViewTabItem.addSubscriber(tab, 'reconnected', function () {
      tab._tabViewTabItem.removeSubscriber(tab, 'reconnected');
      afterAllTabsLoaded(callback);
    });
  }

  let next = function () {
    while (gBrowser.tabs.length-1)
      gBrowser.removeTab(gBrowser.tabs[1]);

    hideTabView(function () {
      let callback = tests.shift();

      if (!callback)
        callback = finish;

      assertOneSingleGroupItem();
      callback();
    });
  }

  
  
  let testUndoCloseTabs = function () {
    gBrowser.loadOneTab('http://mochi.test:8888/', {inBackground: true});
    gBrowser.loadOneTab('http://mochi.test:8888/', {inBackground: true});

    afterAllTabsLoaded(function () {
      assertNumberOfVisibleTabs(3);

      gBrowser.removeTab(gBrowser.tabs[1]);
      gBrowser.selectedTab = gBrowser.tabs[1];

      restoreTab(function () {
        assertNumberOfVisibleTabs(3);
        assertOneSingleGroupItem();
        next();
      });
    });
  }

  
  
  let testDuplicateTab = function () {
    gBrowser.loadOneTab('http://mochi.test:8888/', {inBackground: true});

    afterAllTabsLoaded(function () {
      
      duplicateTabIn(gBrowser.selectedTab, 'tab');

      afterAllTabsLoaded(function () {
        assertNumberOfVisibleTabs(3);
        assertOneSingleGroupItem();
        next();
      });
    });
  }

  
  
  let testBackForwardDuplicateTab = function () {
    let tab = gBrowser.loadOneTab('http://mochi.test:8888/#1', {inBackground: true});
    gBrowser.selectedTab = tab;

    let continueTest = function () {
      tab.linkedBrowser.loadURI('http://mochi.test:8888/#2');

      afterAllTabsLoaded(function () {
        ok(gBrowser.canGoBack, 'browser can go back in history');
        BrowserBack({button: 1});

        afterAllTabsLoaded(function () {
          assertNumberOfVisibleTabs(3);

          ok(gBrowser.canGoForward, 'browser can go forward in history');
          BrowserForward({button: 1});

          afterAllTabsLoaded(function () {
            assertNumberOfVisibleTabs(4);
            assertOneSingleGroupItem();
            next();
          });
        });
      });
    }

    
    
    
    
    afterAllTabsLoaded(function () executeSoon(continueTest));
  }

  
  
  let testPrivateBrowsing = function () {
    gBrowser.loadOneTab('http://mochi.test:8888/#1', {inBackground: true});
    gBrowser.loadOneTab('http://mochi.test:8888/#2', {inBackground: true});

    let cw = getContentWindow();
    let box = new cw.Rect(20, 20, 250, 200);
    let groupItem = new cw.GroupItem([], {bounds: box, immediately: true});
    cw.UI.setActive(groupItem);

    gBrowser.selectedTab = gBrowser.loadOneTab('http://mochi.test:8888/#3', {inBackground: true});
    gBrowser.loadOneTab('http://mochi.test:8888/#4', {inBackground: true});

    afterAllTabsLoaded(function () {
      assertNumberOfVisibleTabs(2);

      enterAndLeavePrivateBrowsing(function () {
        assertNumberOfVisibleTabs(2);
        next();
      });
    });
  }

  waitForExplicitFinish();

  
  tests.push(testUndoCloseTabs);

  
  tests.push(testDuplicateTab);
  tests.push(testBackForwardDuplicateTab);

  
  tests.push(testPrivateBrowsing);

  loadTabView(next);
}


function loadTabView(callback) {
  window.addEventListener('tabviewshown', function () {
    window.removeEventListener('tabviewshown', arguments.callee, false);

    hideTabView(function () {
      window.removeEventListener('tabviewhidden', arguments.callee, false);
      callback();
    });
  }, false);

  TabView.show();
}


function hideTabView(callback) {
  if (!TabView.isVisible())
    return callback();

  window.addEventListener('tabviewhidden', function () {
    window.removeEventListener('tabviewhidden', arguments.callee, false);
    callback();
  }, false);

  TabView.hide();
}


function enterAndLeavePrivateBrowsing(callback) {
  function pbObserver(aSubject, aTopic, aData) {
    if (aTopic != "private-browsing-transition-complete")
      return;

    if (pb.privateBrowsingEnabled)
      pb.privateBrowsingEnabled = false;
    else {
      Services.obs.removeObserver(pbObserver, "private-browsing-transition-complete");
      afterAllTabsLoaded(function () executeSoon(callback));
    }
  }

  Services.obs.addObserver(pbObserver, "private-browsing-transition-complete", false);
  pb.privateBrowsingEnabled = true;
}
