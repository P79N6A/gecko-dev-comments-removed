



function test() {
  waitForExplicitFinish();

  const REDIRECT_URI = NetUtil.newURI("http://mochi.test:8888/tests/toolkit/components/places/tests/browser/redirect.sjs");
  const TARGET_URI = NetUtil.newURI("http://mochi.test:8888/tests/toolkit/components/places/tests/browser/redirect-target.html");

  gBrowser.selectedTab = gBrowser.addTab();
  registerCleanupFunction(function() {
    gBrowser.removeCurrentTab();
  });
  gBrowser.selectedTab.linkedBrowser.loadURI(REDIRECT_URI.spec);

  
  let historyObserver = {
    _redirectNotified: false,
    onVisit: function (aURI, aVisitID, aTime, aSessionID, aReferringID,
                      aTransitionType) {
      info("Received onVisit: " + aURI.spec);

      if (aURI.equals(REDIRECT_URI)) {
        this._redirectNotified = true;
        
        return;
      }

      PlacesUtils.history.removeObserver(historyObserver);

      ok(this._redirectNotified, "The redirect should have been notified");

      fieldForUrl(REDIRECT_URI, "frecency", function (aFrecency) {
        ok(aFrecency != 0, "Frecency or the redirecting page should not be 0");

        fieldForUrl(REDIRECT_URI, "hidden", function (aHidden) {
          is(aHidden, 1, "The redirecting page should be hidden");

          fieldForUrl(TARGET_URI, "frecency", function (aFrecency) {
            ok(aFrecency != 0, "Frecency of the target page should not be 0");

            fieldForUrl(TARGET_URI, "hidden", function (aHidden) {
              is(aHidden, 0, "The target page should not be hidden");

              promiseClearHistory().then(finish);
            });
          });
        });
      });
    },
    onBeginUpdateBatch: function () {},
    onEndUpdateBatch: function () {},
    onTitleChanged: function () {},
    onDeleteURI: function () {},
    onClearHistory: function () {},
    onPageChanged: function () {},
    onDeleteVisits: function () {},
    QueryInterface: XPCOMUtils.generateQI([Ci.nsINavHistoryObserver])
  };
  PlacesUtils.history.addObserver(historyObserver, false);
}
