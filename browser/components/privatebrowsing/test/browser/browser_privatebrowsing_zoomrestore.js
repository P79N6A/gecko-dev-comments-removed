






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
      
      aWindow.FullZoom.enlarge();
      isnot(aWindow.ZoomManager.zoom, 1, "Zoom level for about:blank should be changed");
      aCallback();
      return;
    }
    
    is(aWindow.ZoomManager.zoom, 1, "Zoom level for about:privatebrowsing should be reset");
    aCallback();
  }

  function finishTest() {
    
    windowsToReset.forEach(function(win) {
      win.FullZoom.reset();
    });
    windowsToClose.forEach(function(win) {
      win.close();
    });
    finish();
  }

  function testOnWindow(options, callback) {
    let win = whenNewWindowLoaded(options,
      function(win) {
        windowsToClose.push(win);
        windowsToReset.push(win);
        executeSoon(function() { callback(win); });
      });
  };

  testOnWindow({}, function(win) {
    doTestWhenReady(true, win, function() {
      testOnWindow({private: true}, function(win) {
        doTestWhenReady(false, win, finishTest);
      });
    });
  });
}
