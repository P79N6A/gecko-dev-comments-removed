




































function test() {
  
  
  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  let wm = Cc["@mozilla.org/appshell/window-mediator;1"].
           getService(Ci.nsIWindowMediator);
  waitForExplicitFinish();

  function browserWindowsCount() {
    let count = 0;
    let e = wm.getXULWindowEnumerator("navigator:browser");
    while (e.hasMoreElements()) {
      ++count;
      e.getNext();
    }

    return count;
  }

  is(browserWindowsCount(), 1, "Only one browser window should be open initially");

  
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
        is(browserWindowsCount(), 2, "Two windows should exist at this point");

        
        
        executeSoon(function() {
          ss.setBrowserState(oldState);
        });
      }
      else {
        is(browserWindowsCount(), 1, "Only one window should exist after cleanup");
        os.removeObserver(this, "sessionstore-browser-state-restored");
        finish();
      }
    }
  };
  os.addObserver(observer, "sessionstore-browser-state-restored", false);

  
  ss.setBrowserState(JSON.stringify(testState));
}
