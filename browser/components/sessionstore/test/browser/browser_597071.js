




































function browserWindowsCount() {
  let count = 0;
  let e = Services.wm.getEnumerator("navigator:browser");
  while (e.hasMoreElements()) {
    if (!e.getNext().closed)
      ++count;
  }
  return count;
}

function test() {
  

  waitForExplicitFinish();

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
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
      
      is(browserWindowsCount(), 2, "there should be 2 windows open currently");
      
      let state = JSON.parse(ss.getBrowserState());
      is(state.windows.length, 2, "sessionstore knows about this window");

      newWin.close();
      newWin.addEventListener("unload", function(aEvent) {
        newWin.removeEventListener("unload", arguments.callee, false);

        is(ss.getClosedWindowCount(), closedWindowCount + 1,
           "increased closed window count");
        is(browserWindowsCount(), 1, "there should be 1 window open currently");

        try {
          Services.prefs.clearUserPref("browser.sessionstore.max_windows_undo");
        } catch (e) {}
        ss.setBrowserState(currentState);
        executeSoon(finish);

      }, false);
    }, true);
  }, false);
}

