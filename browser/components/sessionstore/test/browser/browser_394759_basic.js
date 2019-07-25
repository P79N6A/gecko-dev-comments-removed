




































function provideWindow(aCallback, aURL, aFeatures) {
  function callback() {
    executeSoon(function () {
      aCallback(win);
    });
  }

  let win = openDialog(getBrowserURL(), "", aFeatures || "chrome,all,dialog=no", aURL);

  whenWindowLoaded(win, function () {
    if (!aURL) {
      callback();
      return;
    }
    win.gBrowser.selectedBrowser.addEventListener("load", function() {
      win.gBrowser.selectedBrowser.removeEventListener("load", arguments.callee, true);
      callback();
    }, true);
  });
}

function whenWindowLoaded(aWin, aCallback) {
  aWin.addEventListener("load", function () {
    aWin.removeEventListener("load", arguments.callee, false);
    executeSoon(function () {
      aCallback(aWin);
    });
  }, false);
}

function test() {
  waitForExplicitFinish();

  let testURL = "about:config";
  let uniqueKey = "bug 394759";
  let uniqueValue = "unik" + Date.now();
  let uniqueText = "pi != " + Math.random();

  
  let max_windows_undo = gPrefService.getIntPref("browser.sessionstore.max_windows_undo");
  gPrefService.setIntPref("browser.sessionstore.max_windows_undo", max_windows_undo + 1);
  let closedWindowCount = ss.getClosedWindowCount();

  provideWindow(function (newWin) {
    newWin.gBrowser.addTab().linkedBrowser.stop();

    
    ss.setWindowValue(newWin, uniqueKey, uniqueValue);
    let textbox = newWin.content.document.getElementById("textbox");
    textbox.value = uniqueText;

    newWin.close();

    is(ss.getClosedWindowCount(), closedWindowCount + 1,
       "The closed window was added to Recently Closed Windows");
    let data = JSON.parse(ss.getClosedWindowData())[0];
    ok(data.title == testURL && JSON.stringify(data).indexOf(uniqueText) > -1,
       "The closed window data was stored correctly");

    
    let newWin2 = ss.undoCloseWindow(0);

    ok(newWin2 instanceof ChromeWindow,
       "undoCloseWindow actually returned a window");
    is(ss.getClosedWindowCount(), closedWindowCount,
       "The reopened window was removed from Recently Closed Windows");

    
    let restoredTabs = 0;
    let expectedTabs = data.tabs.length;
    whenWindowLoaded(newWin2, function () {
      newWin2.gBrowser.tabContainer.addEventListener("SSTabRestored", function(aEvent) {
        if (++restoredTabs < expectedTabs)
          return;
        newWin2.gBrowser.tabContainer.removeEventListener("SSTabRestored", arguments.callee, true);

        is(newWin2.gBrowser.tabs.length, 2,
           "The window correctly restored 2 tabs");
        is(newWin2.gBrowser.currentURI.spec, testURL,
           "The window correctly restored the URL");

        let textbox = newWin2.content.document.getElementById("textbox");
        is(textbox.value, uniqueText,
           "The window correctly restored the form");
        is(ss.getWindowValue(newWin2, uniqueKey), uniqueValue,
           "The window correctly restored the data associated with it");

        
        newWin2.close();
        gPrefService.clearUserPref("browser.sessionstore.max_windows_undo");
        finish();
      }, true);
    });
  }, testURL);
}
