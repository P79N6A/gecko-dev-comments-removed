






function test() {
  
  waitForExplicitFinish();
  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let observerData;
  function observer(aSubject, aTopic, aData) {
    if (aTopic == "private-browsing")
      observerData = aData;
  }
  Services.obs.addObserver(observer, "private-browsing", false);
  let pbMenuItem = document.getElementById("privateBrowsingItem");
  
  gBrowser.selectedTab = gBrowser.addTab();
  let originalTitle = document.title;

  function testNewWindow(aCallback, expected) {
    Services.obs.addObserver(function observer1(aSubject, aTopic, aData) {
      aSubject.addEventListener("load", function() {
        aSubject.removeEventListener("load", arguments.callee);
        executeSoon(function() {
          let ui = aSubject.gPrivateBrowsingUI;
          is(ui.privateBrowsingEnabled, expected, "The privateBrowsingEnabled property on the new window is set correctly");
          is(PrivateBrowsingUtils.isWindowPrivate(aSubject), expected, "The private bit on the new window is set correctly");

          Services.obs.addObserver(function observer2(aSubject, aTopic, aData) {
            aCallback();
            Services.obs.removeObserver(observer2, "domwindowclosed");
          }, "domwindowclosed", false);
          aSubject.close();
        });
        Services.obs.removeObserver(observer1, "domwindowopened");
      }, false);
    }, "domwindowopened", false);
    OpenBrowserWindow();
  }

  
  ok(gPrivateBrowsingUI, "The gPrivateBrowsingUI object exists");
  is(pb.privateBrowsingEnabled, false, "The private browsing mode should not be started initially");
  is(gPrivateBrowsingUI.privateBrowsingEnabled, false, "gPrivateBrowsingUI should expose the correct private browsing status");
  is(PrivateBrowsingUtils.isWindowPrivate(window), false, "PrivateBrowsingUtils should expose the correct per-window private browsing status");
  ok(pbMenuItem, "The Private Browsing menu item exists");
  is(pbMenuItem.getAttribute("label"), pbMenuItem.getAttribute("startlabel"), "The Private Browsing menu item should read \"Start Private Browsing\"");
  testNewWindow(function() {
    gPrivateBrowsingUI.toggleMode();
    is(pb.privateBrowsingEnabled, true, "The private browsing mode should be started");
    is(gPrivateBrowsingUI.privateBrowsingEnabled, true, "gPrivateBrowsingUI should expose the correct private browsing status");
    is(PrivateBrowsingUtils.isWindowPrivate(window), true, "PrivateBrowsingUtils should expose the correct per-window private browsing status");
    
    is(observerData, "enter", "Private Browsing mode was activated using the gPrivateBrowsingUI object");
    is(pbMenuItem.getAttribute("label"), pbMenuItem.getAttribute("stoplabel"), "The Private Browsing menu item should read \"Stop Private Browsing\"");
    testNewWindow(function() {
      gPrivateBrowsingUI.toggleMode()
      is(pb.privateBrowsingEnabled, false, "The private browsing mode should not be started");
      is(gPrivateBrowsingUI.privateBrowsingEnabled, false, "gPrivateBrowsingUI should expose the correct private browsing status");
      is(PrivateBrowsingUtils.isWindowPrivate(window), false, "PrivateBrowsingUtils should expose the correct per-window private browsing status");
      
      is(observerData, "exit", "Private Browsing mode was deactivated using the gPrivateBrowsingUI object");
      is(pbMenuItem.getAttribute("label"), pbMenuItem.getAttribute("startlabel"), "The Private Browsing menu item should read \"Start Private Browsing\"");

      testNewWindow(function() {
        
        
        setPrivateWindow(window, true);
        is(PrivateBrowsingUtils.isWindowPrivate(window), true, "PrivateBrowsingUtils should accept the correct per-window private browsing status");
        setPrivateWindow(window, false);
        is(PrivateBrowsingUtils.isWindowPrivate(window), false, "PrivateBrowsingUtils should accept the correct per-window private browsing status");

        
        let cmd = document.getElementById("Tools:PrivateBrowsing");
        isnot(cmd, null, "XUL command object for the private browsing service exists");
        var func = new Function("", cmd.getAttribute("oncommand"));
        func.call(cmd);
        
        is(observerData, "enter", "Private Browsing mode was activated using the command object");
        
        isnot(document.title, originalTitle, "Private browsing mode has correctly changed the title");
        func.call(cmd);
        
        is(observerData, "exit", "Private Browsing mode was deactivated using the command object");
        
        is(document.title, originalTitle, "Private browsing mode has correctly restored the title");

        
        gBrowser.removeCurrentTab();
        Services.obs.removeObserver(observer, "private-browsing");
        gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");

        finish();
      }, false);
    }, true);
  }, false);
}
