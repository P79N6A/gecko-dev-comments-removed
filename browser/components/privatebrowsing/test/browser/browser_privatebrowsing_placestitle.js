






add_task(function* test() {
  const TEST_URL = "http://mochi.test:8888/browser/browser/components/" +
                   "privatebrowsing/test/browser/title.sjs";
  let cm = Services.cookies;

  function cleanup() {
    
    cm.removeAll();
    
    return PlacesTestUtils.clearHistory();
  }

  yield cleanup();

  let deferredFirst = PromiseUtils.defer();
  let deferredSecond = PromiseUtils.defer();
  let deferredThird = PromiseUtils.defer();

  let testNumber = 0;
  let historyObserver = {
    onTitleChanged: function(aURI, aPageTitle) {
      if (aURI.spec != TEST_URL)
        return;
      switch (++testNumber) {
        case 1:
          
          deferredFirst.resolve(aPageTitle);
          break;
        case 2:
          
          deferredSecond.resolve(aPageTitle);
          break;
        case 3:
          
          deferredThird.resolve(aPageTitle);
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


  let win = yield BrowserTestUtils.openNewBrowserWindow();
  win.gBrowser.selectedTab = win.gBrowser.addTab(TEST_URL);
  let aPageTitle = yield deferredFirst.promise;
  
  is(aPageTitle, "No Cookie",
     "The page should be loaded without any cookie for the first time");

  win.gBrowser.selectedTab = win.gBrowser.addTab(TEST_URL);
  aPageTitle = yield deferredSecond.promise;
  
  is(aPageTitle, "Cookie",
     "The page should be loaded with a cookie for the second time");

  yield cleanup();

  win.gBrowser.selectedTab = win.gBrowser.addTab(TEST_URL);
  aPageTitle = yield deferredThird.promise;
  
  is(aPageTitle, "No Cookie",
     "The page should be loaded without any cookie again");

  let win2 = yield BrowserTestUtils.openNewBrowserWindow({private: true});

  let private_tab = win2.gBrowser.addTab(TEST_URL);
  win2.gBrowser.selectedTab = private_tab;
  yield BrowserTestUtils.browserLoaded(private_tab.linkedBrowser);

  
  yield cleanup();
  PlacesUtils.history.removeObserver(historyObserver);
  yield BrowserTestUtils.closeWindow(win);
  yield BrowserTestUtils.closeWindow(win2);
});
