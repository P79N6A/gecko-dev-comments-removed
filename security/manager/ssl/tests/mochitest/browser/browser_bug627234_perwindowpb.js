





function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let testURI = "about:blank";
  let uri;
  let gSSService = Cc["@mozilla.org/ssservice;1"].
                   getService(Ci.nsISiteSecurityService);

  function privacyFlags(aIsPrivateMode) {
    return aIsPrivateMode ? Ci.nsISocketProvider.NO_PERMANENT_STORAGE : 0;
  }

  function doTest(aIsPrivateMode, aWindow, aCallback) {
    aWindow.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
      aWindow.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);

      uri = aWindow.Services.io.newURI("https://localhost/img.png", null, null);
      gSSService.processHeader(Ci.nsISiteSecurityService.HEADER_HSTS, uri,
                               "max-age=1000", privacyFlags(aIsPrivateMode));
      ok(gSSService.isSecureHost(Ci.nsISiteSecurityService.HEADER_HSTS, "localhost", privacyFlags(aIsPrivateMode)), "checking sts host");

      aCallback();
    }, true);

    aWindow.gBrowser.selectedBrowser.loadURI(testURI);
  }

  function testOnWindow(aOptions, aCallback) {
    whenNewWindowLoaded(aOptions, function(aWin) {
      windowsToClose.push(aWin);
      
      
      
      executeSoon(function() aCallback(aWin));
    });
  };

   
  registerCleanupFunction(function() {
    windowsToClose.forEach(function(aWin) {
      aWin.close();
    });
    uri = Services.io.newURI("http://localhost", null, null);
    gSSService.removeState(Ci.nsISiteSecurityService.HEADER_HSTS, uri, 0);
  });

  
  testOnWindow({private: true}, function(aWin) {
    doTest(true, aWin, function() {
      
      testOnWindow({}, function(aWin) {
        doTest(false, aWin, function() {
          
          testOnWindow({private: true}, function(aWin) {
            doTest(true, aWin, function () {
              finish();
            });
          });
        });
      });
    });
  });
}
