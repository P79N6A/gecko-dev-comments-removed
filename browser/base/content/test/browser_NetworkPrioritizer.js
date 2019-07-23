




































function waitForActivation(cb, win) {
  if (!win)
    win = window;

  let fm = Cc["@mozilla.org/focus-manager;1"].getService(Ci.nsIFocusManager);
  if (fm.activeWindow == win) {
    cb();
    return;
  }

  win.addEventListener("activate", function () {
    win.removeEventListener("activate", arguments.callee, false);
    executeSoon(cb);
  }, false);
  win.focus();
}

function test() {
  

  waitForExplicitFinish();

  const PRIORITY_DELTA = -10; 

  
  
  function getPriority(aBrowser) {
    
    if (!aBrowser.webNavigation)
      aBrowser = aBrowser.linkedBrowser;
    return aBrowser.webNavigation.QueryInterface(Ci.nsIDocumentLoader)
                   .loadGroup.QueryInterface(Ci.nsISupportsPriority).priority;
  }
  function setPriority(aBrowser, aPriority) {
    if (!aBrowser.webNavigation)
      aBrowser = aBrowser.linkedBrowser;
    aBrowser.webNavigation.QueryInterface(Ci.nsIDocumentLoader)
            .loadGroup.QueryInterface(Ci.nsISupportsPriority).priority = aPriority;
  }

  function isWindowState(aWindow, aTabPriorities) {
    let browsers = aWindow.gBrowser.browsers;
    
    is(browsers.length, aTabPriorities.length,
       "Window has expected number of tabs");
    
    for (let i = 0; i < browsers.length; i++) {
      is(getPriority(browsers[i]), aTabPriorities[i],
         "Tab had expected priority");
    }
  }


  
  
  
  
  function test_behavior() {

    
    let window_A = window;

    
    isWindowState(window_A, [-10]);

    
    let tab_A2 = window_A.gBrowser.addTab("http://example.com");
    let tab_A3 = window_A.gBrowser.addTab("about:config");
    tab_A3.linkedBrowser.addEventListener("load", function(aEvent) {
      tab_A3.removeEventListener("load", arguments.callee, true);

      
      isWindowState(window_A, [-10, 0, 0]);

      
      window_A.gBrowser.selectedTab = tab_A2;
      isWindowState(window_A, [0, -10, 0]);

      window_A.gBrowser.removeTab(tab_A2);
      
      isWindowState(window_A, [0, -10]);

      
      let window_B = openDialog(location, "_blank", "chrome,all,dialog=no", "http://example.com");
      window_B.addEventListener("load", function(aEvent) {
        window_B.removeEventListener("load", arguments.callee, false);
        window_B.gBrowser.addEventListener("load", function(aEvent) {
          window_B.gBrowser.removeEventListener("load", arguments.callee, true);

          waitForActivation(function () {
            isWindowState(window_A, [10, 0]);
            isWindowState(window_B, [-10]);

            waitForActivation(function () {
              isWindowState(window_A, [0, -10]);
              isWindowState(window_B, [0]);

              waitForActivation(function () {
                isWindowState(window_A, [10, 0]);
                isWindowState(window_B, [-10]);

                
                window_B.close();
                window_A.gBrowser.removeTab(tab_A3);
                executeSoon(runNextTest);
              }, window_B);
            }, window_A);
          }, window_B);

        }, true);
      }, false);
    }, true);

  }


  
  
  
  function test_extremePriorities() {
    let tab_A1 = gBrowser.tabContainer.getItemAtIndex(0);
    let oldPriority = getPriority(tab_A1);

    
    
    setPriority(tab_A1, Ci.nsISupportsPriority.PRIORITY_LOWEST);

    let tab_A2 = gBrowser.addTab("http://example.com");
    tab_A2.linkedBrowser.addEventListener("load", function(aEvent) {
      tab_A2.removeEventListener("load", arguments.callee, true);
      gBrowser.selectedTab = tab_A2;
      is(getPriority(tab_A1), Ci.nsISupportsPriority.PRIORITY_LOWEST - PRIORITY_DELTA,
         "Can adjust priority beyond 'lowest'");

      
      setPriority(tab_A1, Ci.nsISupportsPriority.PRIORITY_HIGHEST);
      gBrowser.selectedTab = tab_A1;

      is(getPriority(tab_A1), Ci.nsISupportsPriority.PRIORITY_HIGHEST + PRIORITY_DELTA,
         "Can adjust priority beyond 'highest'");

      
      gBrowser.removeTab(tab_A2);
      executeSoon(function() {
        setPriority(tab_A1, oldPriority);
        runNextTest();
      });

    }, true);
  }


  let tests = [test_behavior, test_extremePriorities];
  function runNextTest() {
    if (tests.length) {
      
      waitForActivation(tests.shift());
    } else {
      finish();
    }
  }

  runNextTest();
}
