



const TEST_URI = NetUtil.newURI("http://mochi.test:8888/notFoundPage.html");

function test() {
  waitForExplicitFinish();

  gBrowser.selectedTab = gBrowser.addTab();
  registerCleanupFunction(function() {
    gBrowser.removeCurrentTab();
  });

  
  
  addVisits({ uri: TEST_URI }, window, () => {
    info("Added visit");
    fieldForUrl(TEST_URI, "frecency", aFrecency => {
      ok(aFrecency > 0, "Frecency should be > 0");
      continueTest(aFrecency);
    });
  });
}

function continueTest(aOldFrecency) {
  
  PlacesUtils.history.markPageAsTyped(TEST_URI);
  gBrowser.selectedBrowser.loadURI(TEST_URI.spec);

  
  let historyObserver = {
    __proto__: NavHistoryObserver.prototype,
    onVisit: function (aURI, aVisitID, aTime, aSessionID, aReferringID,
                      aTransitionType) {
      PlacesUtils.history.removeObserver(historyObserver);
      info("Received onVisit: " + aURI.spec);
      fieldForUrl(aURI, "frecency", function (aFrecency) {
        is(aFrecency, aOldFrecency, "Frecency should be unchanged");
        fieldForUrl(aURI, "hidden", function (aHidden) {
          is(aHidden, 0, "Page should not be hidden");
          fieldForUrl(aURI, "typed", function (aTyped) {
            is(aTyped, 0, "page should not be marked as typed");
            promiseClearHistory().then(finish);
          });
        });
      });
    }
  };
  PlacesUtils.history.addObserver(historyObserver, false);
}
