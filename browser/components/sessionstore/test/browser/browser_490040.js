




































function browserWindowsCount() {
  let count = 0;
  let e = Cc["@mozilla.org/appshell/window-mediator;1"]
            .getService(Ci.nsIWindowMediator)
            .getEnumerator("navigator:browser");
  while (e.hasMoreElements()) {
    if (!e.getNext().closed)
      ++count;
  }
  return count;
}

function test() {
  
  is(browserWindowsCount(), 1, "Only one browser window should be open initially");

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);

  waitForExplicitFinish();

  function testWithState(aState) {
    
    let curClosedWindowCount = ss.getClosedWindowCount();
    gPrefService.setIntPref("browser.sessionstore.max_windows_undo",
                            curClosedWindowCount + 1);

    var origWin;
    let windowObserver = {
      observe: function(aSubject, aTopic, aData) {
        let theWin = aSubject.QueryInterface(Ci.nsIDOMWindow);
        if (origWin && theWin != origWin)
          return;

        switch (aTopic) {
          case "domwindowopened":
            origWin = theWin;
            theWin.addEventListener("load", function () {
              theWin.removeEventListener("load", arguments.callee, false);
              executeSoon(function() {
                
                
                if (aState.windowState.windows[0].tabs[0].entries.length) {
                  theWin.gBrowser.addEventListener("load", function() {
                    theWin.gBrowser.removeEventListener("load",
                                                        arguments.callee, true);
                    theWin.close();
                  }, true);
                } else {
                  executeSoon(function() {
                    theWin.close();
                  });
                }
                ss.setWindowState(theWin, JSON.stringify(aState.windowState),
                                  true);
              });
            }, false);
            break;

          case "domwindowclosed":
            ww.unregisterNotification(this);
            
            executeSoon(function() {
              is(ss.getClosedWindowCount(),
                 curClosedWindowCount + (aState.shouldBeAdded ? 1 : 0),
                 "That window should " + (aState.shouldBeAdded ? "" : "not ") +
                 "be restorable");
              executeSoon(runNextTest);
            });
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

  
  
  let states = [
    {
      shouldBeAdded: true,
      windowState: {
        windows: [{
          tabs: [{ entries: [{ url: "http://example.com", title: "example.com" }] }],
          selected: 1,
          _closedTabs: []
        }]
      }
    },
    {
      shouldBeAdded: false,
      windowState: {
        windows: [{
          tabs: [{ entries: [] }],
          _closedTabs: []
        }]
      }
    },
    {
      shouldBeAdded: false,
      windowState: {
        windows: [{
          tabs: [{ entries: [] }],
          _closedTabs: [{ state: { entries: [{ url: "http://example.com", index: 1 }] } }]
        }]
      }
    },
    {
      shouldBeAdded: false,
      windowState: {
        windows: [{
          tabs: [{ entries: [] }],
          _closedTabs: [],
          extData: { keyname: "pi != " + Math.random() }
        }]
      }
    }
  ];

  function runNextTest() {
    if (states.length) {
      let state = states.shift();
      testWithState(state);
    }
    else {
      if (gPrefService.prefHasUserValue("browser.sessionstore.max_windows_undo"))
        gPrefService.clearUserPref("browser.sessionstore.max_windows_undo");
      is(browserWindowsCount(), 1, "Only one browser window should be open eventually");
      finish();
    }
  }
  runNextTest();
}

