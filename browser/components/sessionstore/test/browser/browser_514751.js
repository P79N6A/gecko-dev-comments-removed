



































function test() {
  

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);

  waitForExplicitFinish();

  let state = {
    windows: [{
      tabs: [{
        entries: [
          { url: "http://www.mozilla.org/projects/minefield/", title: "Minefield Start Page" },
          {}
        ]
      }]
    }]
  };

  let windowObserver = {
    observe: function(aSubject, aTopic, aData) {
      let theWin = aSubject.QueryInterface(Ci.nsIDOMWindow);

      switch(aTopic) {
        case "domwindowopened":
          theWin.addEventListener("load", function () {
            theWin.removeEventListener("load", arguments.callee, false);
            executeSoon(function() {
              var gotError = false;
              try {
                ss.setWindowState(theWin, JSON.stringify(state), true);
              } catch (e) {
                if (/NS_ERROR_MALFORMED_URI/.test(e))
                  gotError = true;
              }
              ok(!gotError, "Didn't get a malformed URI error.");
              executeSoon(function() {
                theWin.close();
              });
            });
          }, false);
          break;

        case "domwindowclosed":
          ww.unregisterNotification(this);
          finish();
          break;
      }
    }
  }
  ww.registerNotification(windowObserver);
  ww.openWindow(null,
                location,
                "_blank",
                "chrome,all,dialog=no",
                null);

}

