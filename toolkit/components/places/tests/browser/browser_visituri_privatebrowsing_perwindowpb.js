



function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let testURI = "https://www.mozilla.org/en-US/";
  let initialURL =
    "http://example.com/tests/toolkit/components/places/tests/browser/begin.html";
  let finalURL =
    "http://example.com/tests/toolkit/components/places/tests/browser/final.html";
  let observer = null;
  let enumerator = null;
  let currentObserver = null;
  let uri = null;

  function doTest(aIsPrivateMode, aWindow, aTestURI, aCallback) {
    aWindow.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
      aWindow.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);

      if (aCallback) {
        aCallback();
      }
    }, true);

    observer = {
      observe: function(aSubject, aTopic, aData) {
        
        if (aTopic == "uri-visit-saved") {
          
          
          enumerator = aWindow.Services.obs.enumerateObservers("uri-visit-saved");
          while (enumerator.hasMoreElements()) {
            currentObserver = enumerator.getNext();
            aWindow.Services.obs.removeObserver(currentObserver, "uri-visit-saved");
          }

          
          
          uri = aSubject.QueryInterface(Ci.nsIURI);
          is(uri.spec, finalURL, "Check received expected visit");
        }
      }
    };

    aWindow.Services.obs.addObserver(observer, "uri-visit-saved", false);
    aWindow.gBrowser.selectedBrowser.loadURI(aTestURI);
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
  });

  
  testOnWindow({private: true}, function(aWin) {
    doTest(true, aWin, initialURL, function() {
      
      testOnWindow({}, function(aWin) {
        doTest(false, aWin, finalURL, function () {
          promiseClearHistory().then(finish);
        });
      });
    });
  });
}
