




































function test() {
  

  waitForExplicitFinish();

  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  
  
  let closedWindowCount = ss.getClosedWindowCount();
  Services.prefs.setIntPref("browser.sessionstore.max_windows_undo",
                            closedWindowCount + 1);

  let currentState = ss.getBrowserState();
  let popupState = { windows:[
    { tabs:[ {entries:[] }], isPopup: true, hidden: "toolbar" }
  ] };

  
  ss.setWindowState(window, JSON.stringify(popupState), true);

  
  let newWin = openDialog(location, "", "chrome,all,dialog=no", "http://example.com");
  newWin.addEventListener("load", function(aEvent) {
    newWin.removeEventListener("load", arguments.callee, false);

    newWin.gBrowser.addEventListener("load", function(aEvent) {
      newWin.gBrowser.removeEventListener("load", arguments.callee, true);

      newWin.gBrowser.addTab().linkedBrowser.stop();

      
      let state = JSON.parse(ss.getBrowserState());
      is(state.windows.length, 2, "sessionstore knows about this window");

      newWin.close();
      newWin.addEventListener("unload", function(aEvent) {
        newWin.removeEventListener("unload", arguments.callee, false);

        is(ss.getClosedWindowCount(), closedWindowCount + 1,
           "increased closed window count");

        Services.prefs.clearUserPref("browser.sessionstore.max_windows_undo");
        ss.setBrowserState(currentState);
        executeSoon(finish);

      }, false);
    }, true);
  }, false);
}

