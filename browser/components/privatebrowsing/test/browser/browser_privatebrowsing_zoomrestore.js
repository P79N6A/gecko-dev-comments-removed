






function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let windowsToReset = [];

  
  
  function debugLogCPS(aWindow, aCallback) {
    let pb = { usePrivateBrowsing: true };
    let queue = [
      ["getByDomainAndName", ["about:blank", aWindow.FullZoom.name, null]],
      ["getByDomainAndName", ["about:blank", aWindow.FullZoom.name, pb]],
      ["getGlobal", [aWindow.FullZoom.name, null]],
      ["getGlobal", [aWindow.FullZoom.name, pb]],
    ];
    debugCallCPS(queue, aCallback);
  }
  function debugCallCPS(aQueue, aCallback) {
    if (!aQueue.length) {
      aCallback();
      return;
    }
    let [methodName, args] = aQueue.shift();
    let argsDup = args.slice();
    argsDup.push({
      handleResult: function debug_handleResult(pref) {
        info("handleResult " + methodName + "(" + args.toSource() +
             ") pref=" + pref.value);
      },
      handleCompletion: function debug_handleCompletion() {
        info("handleCompletion " + methodName + "(" + args.toSource() + ")");
        debugCallCPS(aQueue, aCallback);
      },
    });
    let cps = Cc["@mozilla.org/content-pref/service;1"].
              getService(Ci.nsIContentPrefService2);
    cps[methodName].apply(cps, argsDup);
  }

  function doTest(aIsZoomedWindow, aWindow, aCallback) {
    aWindow.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
      aWindow.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
      if (aIsZoomedWindow) {
        info("about:blank zoom level should be 1: " + aWindow.ZoomManager.zoom);
        if (aWindow.ZoomManager.zoom != 1) {
          
          
          
          
          info("zoom level != 1, logging content prefs");
          debugLogCPS(aWindow, aCallback);
          return;
        }
        
        aWindow.FullZoom.enlarge(function () {
          isnot(aWindow.ZoomManager.zoom, 1, "Zoom level for about:blank should be changed");
          aCallback();
        });
        return;
      }
      
      is(aWindow.ZoomManager.zoom, 1, "Zoom level for about:privatebrowsing should be reset");

      aCallback();
    }, true);

    aWindow.gBrowser.selectedBrowser.loadURI("about:blank");
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
    doTest(true, win, function() {
      testOnWindow({private: true}, function(win) {
        doTest(false, win, finishTest);
      });
    });
  });
}
