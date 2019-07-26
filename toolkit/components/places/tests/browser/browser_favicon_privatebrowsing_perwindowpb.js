



function test() {
  waitForExplicitFinish();

  const pageURI =
   "http://example.org/tests/toolkit/components/places/tests/browser/favicon.html";
  let windowsToClose = [];

  registerCleanupFunction(function() {
    windowsToClose.forEach(function(aWin) {
      aWin.close();
    });
  });

  function testOnWindow(aIsPrivate, aCallback) {
    whenNewWindowLoaded({private: aIsPrivate}, function(aWin) {
      windowsToClose.push(aWin);
      executeSoon(function() aCallback(aWin));
    });
  };

  function waitForTabLoad(aWin, aCallback) {
    aWin.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
      aWin.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
      aCallback();
    }, true);
    aWin.gBrowser.selectedBrowser.loadURI(pageURI);
  }

  testOnWindow(true, function(win) {
    waitForTabLoad(win, function() {
      PlacesUtils.favicons.getFaviconURLForPage(NetUtil.newURI(pageURI),
        function(uri, dataLen, data, mimeType) {
          is(uri, null, "No result should be found");
          finish();
        }
      );
    });
  });
}
