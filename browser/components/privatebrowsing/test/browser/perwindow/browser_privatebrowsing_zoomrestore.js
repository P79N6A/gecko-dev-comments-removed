






function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let windowsToReset = [];

  function doTest(aIsZoomedWindow, aWindow, aCallback) {
    aWindow.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
      aWindow.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
      if (aIsZoomedWindow) {
        
        aWindow.FullZoom.enlarge();
        isnot(aWindow.ZoomManager.zoom, 1, "Zoom level for about:blank should be changed");
      } else {
        
        is(aWindow.ZoomManager.zoom, 1, "Zoom level for about:privatebrowsing should be reset");
      }

      aCallback();
    }, true);

    aWindow.gBrowser.selectedBrowser.loadURI("about:blank");
  }

  function finishTest() {
    
    windowsToReset.forEach(function(win) {
      win.FullZoom.reset();
    });
    finish();
  }

  function testOnWindow(options, callback) {
    let win = OpenBrowserWindow(options);
    win.addEventListener("load", function onLoad() {
      win.removeEventListener("load", onLoad, false);
      windowsToClose.push(win);
      windowsToReset.push(win);
      executeSoon(function() callback(win));
    }, false);
  };

  registerCleanupFunction(function() {
    windowsToClose.forEach(function(win) {
      win.close();
    });
  });

  testOnWindow({}, function(win) {
    doTest(true, win, function() {
      testOnWindow({private: true}, function(win) {
        doTest(false, win, finishTest);
      });
    });
  });
}
