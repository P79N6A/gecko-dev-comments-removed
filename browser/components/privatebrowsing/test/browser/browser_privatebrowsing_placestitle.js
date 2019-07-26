






function test() {
  waitForExplicitFinish();

  const TEST_URL = "http://mochi.test:8888/browser/browser/components/" +
                   "privatebrowsing/test/browser/title.sjs";
  let cm = Cc["@mozilla.org/cookiemanager;1"].getService(Ci.nsICookieManager);

  function waitForCleanup(aCallback) {
    
    cm.removeAll();
    
    Services.obs.addObserver(function observeCH(aSubject, aTopic, aData) {
      Services.obs.removeObserver(observeCH, PlacesUtils.TOPIC_EXPIRATION_FINISHED);
      aCallback();
    }, PlacesUtils.TOPIC_EXPIRATION_FINISHED, false);
    PlacesUtils.bhistory.removeAllPages();
  }

  let testNumber = 0;
  let historyObserver = {
    onTitleChanged: function(aURI, aPageTitle) {
      if (aURI.spec != TEST_URL)
        return;
      switch (++testNumber) {
        case 1:
          
          is(aPageTitle, "No Cookie",
             "The page should be loaded without any cookie for the first time");
          openTestPage(selectedWin);
          break;
        case 2:
          
          is(aPageTitle, "Cookie",
             "The page should be loaded with a cookie for the second time");
          waitForCleanup(function () {
            openTestPage(selectedWin);
          });
          break;
        case 3:
          
          is(aPageTitle, "No Cookie",
             "The page should be loaded without any cookie again");
          testOnWindow(true, function(win) {
            whenPageLoad(win, function() {
              waitForCleanup(finish);
            });
          });
          break;
        default:
          
          
          ok(false, "Title changed. Unexpected pass: " + testNumber);
      }
    },

    onBeginUpdateBatch: function () {},
    onEndUpdateBatch: function () {},
    onVisit: function () {},
    onDeleteURI: function () {},
    onClearHistory: function () {},
    onPageChanged: function () {},
    onDeleteVisits: function() {},
    QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver])
  };
  PlacesUtils.history.addObserver(historyObserver, false);

  let selectedWin = null;
  let windowsToClose = [];
  registerCleanupFunction(function() {
    PlacesUtils.history.removeObserver(historyObserver);
    windowsToClose.forEach(function(win) {
      win.close();
    });
  });

  function openTestPage(aWin) {
    aWin.gBrowser.selectedTab = aWin.gBrowser.addTab(TEST_URL);
  }

  function whenPageLoad(aWin, aCallback) {
    aWin.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
      aWin.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
      aCallback();
    }, true);
    aWin.gBrowser.selectedBrowser.loadURI(TEST_URL);
  }

  function testOnWindow(aPrivate, aCallback) {
    whenNewWindowLoaded({ private: aPrivate }, function(win) {
      selectedWin = win;
      windowsToClose.push(win);
      executeSoon(function() { aCallback(win) });
    });
  }

  waitForCleanup(function() {
    testOnWindow(false, function(win) {
      openTestPage(win);
    });
  });
}
