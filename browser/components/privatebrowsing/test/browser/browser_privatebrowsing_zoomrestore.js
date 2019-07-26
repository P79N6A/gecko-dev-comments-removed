






function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let windowsToReset = [];

  function doTestWhenReady(aIsZoomedWindow, aWindow, aCallback) {
    
    
    
    

    let n = 0;

    let browser = aWindow.gBrowser.selectedBrowser;
    browser.addEventListener("load", function onLoad() {
      browser.removeEventListener("load", onLoad, true);
      if (++n == 2)
        doTest(aIsZoomedWindow, aWindow, aCallback);
    }, true);

    let topic = "browser-fullZoom:locationChange";
    Services.obs.addObserver(function onLocationChange() {
      Services.obs.removeObserver(onLocationChange, topic);
      if (++n == 2)
        doTest(aIsZoomedWindow, aWindow, aCallback);
    }, topic, false);

    browser.loadURI("about:blank");
  }

  function doTest(aIsZoomedWindow, aWindow, aCallback) {
    if (aIsZoomedWindow) {
      is(aWindow.ZoomManager.zoom, 1,
         "Zoom level for freshly loaded about:blank should be 1");
      
      aWindow.FullZoom.enlarge(function () {
        isnot(aWindow.ZoomManager.zoom, 1, "Zoom level for about:blank should be changed");
        aCallback();
      });
      return;
    }
    
    is(aWindow.ZoomManager.zoom, 1, "Zoom level for about:privatebrowsing should be reset");
    aCallback();
  }

  function finishTest() {
    
    let numWindows = windowsToReset.length;
    if (!numWindows) {
      finish();
      return;
    }
    windowsToReset.forEach(function(win) {
      win.FullZoom.reset(function onReset() {
        numWindows--;
        if (!numWindows)
          finish();
      });
    });
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
    doTestWhenReady(true, win, function() {
      testOnWindow({private: true}, function(win) {
        doTestWhenReady(false, win, finishTest);
      });
    });
  });
}
