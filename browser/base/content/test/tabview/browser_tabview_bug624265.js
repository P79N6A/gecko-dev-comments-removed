


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

    afterAllTabsLoaded(function () {
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
    });
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
  showTabView(function () {
    hideTabView(callback);
  });
}


function enterAndLeavePrivateBrowsing(callback) {
  togglePrivateBrowsing(function () {
    togglePrivateBrowsing(callback);
  });
}


function togglePrivateBrowsing(callback) {
  let topic = "private-browsing-transition-complete";

  function pbObserver(aSubject, aTopic, aData) {
    if (aTopic != topic)
      return;

    Services.obs.removeObserver(pbObserver, topic, false);
    afterAllTabsLoaded(callback);
  }

  Services.obs.addObserver(pbObserver, topic, false);
  pb.privateBrowsingEnabled = !pb.privateBrowsingEnabled;
}
