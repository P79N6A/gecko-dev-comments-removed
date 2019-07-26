






function test() {
  
  waitForExplicitFinish();
  let windowsToClose = [];
  let testURI = "about:blank";
  let pbMenuItem;
  let cmd;

  function doTest(aIsPrivateMode, aWindow, aCallback) {
    aWindow.gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
      aWindow.gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);

      ok(aWindow.gPrivateBrowsingUI, "The gPrivateBrowsingUI object exists");

      pbMenuItem = aWindow.document.getElementById("menu_newPrivateWindow");
      ok(pbMenuItem, "The Private Browsing menu item exists");

      cmd = aWindow.document.getElementById("Tools:PrivateBrowsing");
      isnot(cmd, null, "XUL command object for the private browsing service exists");

      is(pbMenuItem.getAttribute("label"), "New Private Window",
        "The Private Browsing menu item should read \"New Private Window\"");
      is(PrivateBrowsingUtils.isWindowPrivate(aWindow), aIsPrivateMode,
        "PrivateBrowsingUtils should report the correct per-window private browsing status (privateBrowsing should be " +
        aIsPrivateMode + ")");

      aCallback();
    }, true);

    aWindow.gBrowser.selectedBrowser.loadURI(testURI);
  };

  function openPrivateBrowsingModeByUI(aWindow, aCallback) {
    Services.obs.addObserver(function observer(aSubject, aTopic, aData) {
      aSubject.addEventListener("load", function() {
        aSubject.removeEventListener("load", arguments.callee);
          Services.obs.removeObserver(observer, "domwindowopened");
          windowsToClose.push(aSubject);
          aCallback(aSubject);
      }, false);
    }, "domwindowopened", false);

    cmd = aWindow.document.getElementById("Tools:PrivateBrowsing");
    var func = new Function("", cmd.getAttribute("oncommand"));
    func.call(cmd);
  };

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

  
  testOnWindow({}, function(aWin) {
    doTest(false, aWin, function() {
      
      
      openPrivateBrowsingModeByUI(aWin, function(aPrivateWin) {
        doTest(true, aPrivateWin, finish);
      });
    });
  });
}
