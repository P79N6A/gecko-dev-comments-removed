




































function test() {
  

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  let os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);

  waitForExplicitFinish();

  function testWithState(aState, aCallback) {
    
    let curClosedWindowCount = ss.getClosedWindowCount();
    gPrefService.setIntPref("browser.sessionstore.max_windows_undo", curClosedWindowCount + 1);

    let theWin = openDialog(location, "_blank", "chrome,all,dialog=no");
    theWin.addEventListener("load", function(aEvent) {
      theWin.gBrowser.removeEventListener("load", arguments.callee, true);

      ss.setWindowState(theWin, JSON.stringify(aState.windowState), true);

      let observer = {
        QueryInterface: XPCOMUtils.generateQI([Ci.nsIObserver,
                                               Ci.nsISupportsWeakReference]),
        observe: function(aSubject, aTopic, aData) {
          let _this = this;
          
          executeSoon(function() {
            is(ss.getClosedWindowCount(), curClosedWindowCount + (aState.shouldBeAdded ? 1 : 0),
               "That window should " + (aState.shouldBeAdded ? "" : "not ") + "be restorable");
            os.removeObserver(_this, "domwindowclosed");
            executeSoon(aCallback);
          });
        }
      };
      os.addObserver(observer, "domwindowclosed", true);

      theWin.gBrowser.addEventListener("load", function() {
        theWin.gBrowser.removeEventListener("load", arguments.callee, true);
        theWin.close();
      }, true);
    }, true);
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

  testWithState(states[0], function() {
    testWithState(states[1], function() {
      testWithState(states[2], function() {
        testWithState(states[3], function() {
          gPrefService.clearUserPref("browser.sessionstore.max_windows_undo");
          finish();
        });
      });
    });
  });
}

