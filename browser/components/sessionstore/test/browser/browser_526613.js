




































function test() {
  
  
  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  let wm = Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(Ci.nsIWindowMediator);
  waitForExplicitFinish();

  function browserWindowsCount(expected) {
    let count = 0;
    let e = wm.getEnumerator("navigator:browser");
    while (e.hasMoreElements()) {
      if (!e.getNext().closed)
        ++count;
    }
    is(count, expected,
       "number of open browser windows according to nsIWindowMediator");
    let state = ss.getBrowserState();
    info(state);
    is(JSON.parse(state).windows.length, expected,
       "number of open browser windows according to getBrowserState");
  }

  browserWindowsCount(1);

  
  let oldState = ss.getBrowserState();
  
  let testState = {
    windows: [
      { tabs: [{ entries: [{ url: "http://example.com/" }] }], selected: 1 },
      { tabs: [{ entries: [{ url: "about:robots"        }] }], selected: 1 },
    ],
    
    
    selectedWindow: 1
  };

  let observer = {
    pass: 1,
    observe: function(aSubject, aTopic, aData) {
      is(aTopic, "sessionstore-browser-state-restored",
         "The sessionstore-browser-state-restored notification was observed");

      if (this.pass++ == 1) {
        browserWindowsCount(2);

        
        function pollMostRecentWindow() {
          if (wm.getMostRecentWindow("navigator:browser") == window) {
            ss.setBrowserState(oldState);
          } else {
            info("waiting for the current window to become active");
            setTimeout(pollMostRecentWindow, 0);
          }
        }
        window.focus(); 
        pollMostRecentWindow();
      }
      else {
        browserWindowsCount(1);
        ok(!window.closed, "Restoring the old state should have left this window open");
        os.removeObserver(this, "sessionstore-browser-state-restored");
        finish();
      }
    }
  };
  os.addObserver(observer, "sessionstore-browser-state-restored", false);

  
  ss.setBrowserState(JSON.stringify(testState));
}
