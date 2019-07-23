










































































































function browserWindowsCount(expected, msg) {
  if (typeof expected == "number")
    expected = [expected, expected];
  let count = 0;
  let e = Cc["@mozilla.org/appshell/window-mediator;1"]
            .getService(Ci.nsIWindowMediator)
            .getEnumerator("navigator:browser");
  while (e.hasMoreElements()) {
    if (!e.getNext().closed)
      ++count;
  }
  is(count, expected[0], msg + " (nsIWindowMediator)");
  let state = Cc["@mozilla.org/browser/sessionstore;1"]
                .getService(Ci.nsISessionStore)
                .getBrowserState();
  info(state);
  is(JSON.parse(state).windows.length, expected[1], msg + " (getBrowserState)");
}

function test() {
  browserWindowsCount(1, "Only one browser window should be open initially");

  waitForExplicitFinish();

  
  
  
  const TEST_URLS = ["about:mozilla", "about:buildconfig"];

  
  
  const NOTIFICATIONS_EXPECTED = 6;

  
  const POPUP_FEATURES = "toolbar=no,resizable=no,status=no";

  
  const CHROME_FEATURES = "chrome,all,dialog=no";

  
  let oldWinType = "";
  
  
  let oldWarnTabsOnClose = gPrefService.getBoolPref("browser.tabs.warnOnClose");

  
  let observing = {
    "browser-lastwindow-close-requested": 0,
    "browser-lastwindow-close-granted": 0
  };

  


  let observer = {
    hitCount: 0,

    observe: function(aCancel, aTopic, aData) {
      
      observing[aTopic]++;

      
      if (++this.hitCount == 1) {
        
        aCancel.QueryInterface(Ci.nsISupportsPRBool).data = true;
      }
    }
  };
  let observerService = Cc["@mozilla.org/observer-service;1"].
                        getService(Ci.nsIObserverService);

  



  function setPrefs() {
    gPrefService.setIntPref("browser.startup.page", 3);
    gPrefService.setBoolPref(
      "browser.privatebrowsing.keep_current_session",
      true
    );
    gPrefService.setBoolPref("browser.tabs.warnOnClose", false);
  }

  


  function setupTestsuite(testFn) {
    
    for (let o in observing) {
      observerService.addObserver(observer, o, false);
    }

    
    oldWinType = document.documentElement.getAttribute("windowtype");
    document.documentElement.setAttribute("windowtype", "navigator:testrunner");
  }

  


  function cleanupTestsuite(callback) {
    
    for (let o in observing) {
      observerService.removeObserver(observer, o, false);
    }
    
    for each (let pref in [
      "browser.startup.page",
      "browser.privatebrowsing.keep_current_session"
    ]) {
      if (gPrefService.prefHasUserValue(pref)) {
        gPrefService.clearUserPref(pref);
      }
    }
    gPrefService.setBoolPref("browser.tabs.warnOnClose", oldWarnTabsOnClose);

    
    document.documentElement.setAttribute("windowtype", oldWinType);
  }

  


  function setupTestAndRun(testFn) {
    
    setPrefs();

    
    let newWin = openDialog(location, "_blank", CHROME_FEATURES, "about:config");
    newWin.addEventListener("load", function(aEvent) {
      newWin.removeEventListener("load", arguments.callee, false);
      newWin.gBrowser.addEventListener("load", function(aEvent) {
        newWin.gBrowser.removeEventListener("load", arguments.callee, true);
        TEST_URLS.forEach(function (url) {
          newWin.gBrowser.addTab(url);
        });

        executeSoon(function() testFn(newWin));
      }, true);
    }, false);
  }

  



  function testOpenCloseNormal(nextFn) {
    setupTestAndRun(function(newWin) {
      
      
      
      newWin.BrowserTryToCloseWindow();

      
      ok(!newWin.closed, "First close request was denied");
      if (!newWin.closed) {
        newWin.BrowserTryToCloseWindow();
        ok(newWin.closed, "Second close request was granted");
      }

      
      
      newWin = openDialog(location, "_blank", CHROME_FEATURES);
      newWin.addEventListener("load", function() {
        this.removeEventListener("load", arguments.callee, true);
        executeSoon(function() {
          is(newWin.gBrowser.browsers.length, TEST_URLS.length + 1,
             "Restored window in-session with otherpopup windows around");

          
          newWin.close();

          
          executeSoon(nextFn);
        });
      }, true);
    });
  }

  



  function testOpenClosePrivateBrowsing(nextFn) {
    setupTestAndRun(function(newWin) {
      let pb = Cc["@mozilla.org/privatebrowsing;1"].
               getService(Ci.nsIPrivateBrowsingService);

      
      newWin.BrowserTryToCloseWindow();

      
      pb.privateBrowsingEnabled = true;

      
      
      newWin = openDialog(location, "_blank", CHROME_FEATURES);
      newWin.addEventListener("load", function() {
        this.removeEventListener("load", arguments.callee, true);
        executeSoon(function() {
          is(newWin.gBrowser.browsers.length, 1,
             "Did not restore in private browing mode");

          
          newWin.BrowserTryToCloseWindow();

          
          pb.privateBrowsingEnabled = false;

          newWin = openDialog(location, "_blank", CHROME_FEATURES);
          newWin.addEventListener("load", function() {
            this.removeEventListener("load", arguments.callee, true);
            executeSoon(function() {
              is(newWin.gBrowser.browsers.length, TEST_URLS.length + 1,
                 "Restored after leaving private browsing again");

              newWin.close();

              
              executeSoon(nextFn);
            });
          }, true);
        });
      }, true);
    });
  }

  




  function testOpenCloseWindowAndPopup(nextFn) {
    setupTestAndRun(function(newWin) {
      
      let popup = openDialog(location, "popup", POPUP_FEATURES, TEST_URLS[0]);
      let popup2 = openDialog(location, "popup2", POPUP_FEATURES, TEST_URLS[1]);
      popup2.addEventListener("load", function() {
        popup2.removeEventListener("load", arguments.callee, false);
        popup2.gBrowser.addEventListener("load", function() {
          popup2.gBrowser.removeEventListener("load", arguments.callee, true);
          popup2.gBrowser.addTab(TEST_URLS[0]);
          
          newWin.BrowserTryToCloseWindow();

          
          
          
          popup2.close();

          
          newWin = openDialog(location, "_blank", CHROME_FEATURES);
          newWin.addEventListener("load", function() {
            this.removeEventListener("load", arguments.callee, true);
            executeSoon(function() {
              is(newWin.gBrowser.browsers.length, TEST_URLS.length + 1,
                 "Restored window and associated tabs in session");

              
              newWin.close();
              popup.close();

              
              executeSoon(nextFn);
            });
          }, true);
        }, true);
      }, false);
    });
  }

  




  function testOpenCloseOnlyPopup(nextFn) {
    
    setPrefs();

    
    
    let popup = openDialog(location, "popup", POPUP_FEATURES, TEST_URLS[1]);
    popup.addEventListener("load", function() {
      this.removeEventListener("load", arguments.callee, true);
      is(popup.gBrowser.browsers.length, 1,
         "Did not restore the popup window (1)");
      popup.BrowserTryToCloseWindow();

      
      popup = openDialog(location, "popup", POPUP_FEATURES, TEST_URLS[1]);
      popup.addEventListener("load", function() {
        popup.removeEventListener("load", arguments.callee, false);
        popup.gBrowser.addEventListener("load", function() {
          popup.gBrowser.removeEventListener("load", arguments.callee, true);
          popup.gBrowser.addTab(TEST_URLS[0]);

          is(popup.gBrowser.browsers.length, 2,
             "Did not restore to the popup window (2)");

          
          
          
          popup.close();

          let newWin = openDialog(location, "_blank", CHROME_FEATURES, "about:blank");
          newWin.addEventListener("load", function() {
            newWin.removeEventListener("load", arguments.callee, true);
            executeSoon(function() {
              isnot(newWin.gBrowser.browsers.length, 2,
                    "Did not restore the popup window");
              is(TEST_URLS.indexOf(newWin.gBrowser.browsers[0].currentURI.spec), -1,
                 "Did not restore the popup window (2)");

              
              newWin.close();

              
              executeSoon(nextFn);
            });
          }, true);
        }, true);
      }, false);
    }, true);
  }

    




  function testOpenCloseRestoreFromPopup(nextFn) {
    setupTestAndRun(function(newWin) {
      setupTestAndRun(function(newWin2) {
        newWin.BrowserTryToCloseWindow();
        newWin2.BrowserTryToCloseWindow();

        newWin = undoCloseWindow(0);
        newWin.addEventListener("load", function () {
          info(["testOpenCloseRestoreFromPopup: newWin loaded", newWin.closed, newWin.document]);
          var ds = newWin.delayedStartup;
          newWin.delayedStartup = function () {
            info(["testOpenCloseRestoreFromPopup: newWin delayedStartup", newWin.closed, newWin.document]);
            ds.apply(newWin, arguments);
          };
        }, false);

        newWin2 = openDialog(location, "_blank", CHROME_FEATURES);
        newWin2.addEventListener("load", function() {
          newWin2.removeEventListener("load", arguments.callee, true);
          executeSoon(function() {
            is(newWin2.gBrowser.browsers.length, 1,
               "Did not restore, as undoCloseWindow() was last called");
            is(TEST_URLS.indexOf(newWin2.gBrowser.browsers[0].currentURI.spec), -1,
               "Did not restore, as undoCloseWindow() was last called (2)");

            
            newWin.close();
            newWin2.close();

            
            executeSoon(nextFn);
          });
        }, true);
      });
    });
  }

  



  function testNotificationCount(nextFn) {
    is(observing["browser-lastwindow-close-requested"], NOTIFICATIONS_EXPECTED,
       "browser-lastwindow-close-requested notifications observed");

    
    
    
    is(observing["browser-lastwindow-close-requested"],
       observing["browser-lastwindow-close-granted"] + 1,
       "Notification count for -request and -grant matches");

    executeSoon(nextFn);
  }

  




  function testMacNotifications(nextFn, iteration) {
    iteration = iteration || 1;
    setupTestAndRun(function(newWin) {
      
      
      
      newWin.BrowserTryToCloseWindow();
      if (iteration == 1) {
        ok(!newWin.closed, "First close attempt denied");
        if (!newWin.closed) {
          newWin.BrowserTryToCloseWindow();
          ok(newWin.closed, "Second close attempt granted");
        }
      }

      if (iteration < NOTIFICATIONS_EXPECTED - 1) {
        executeSoon(function() testMacNotifications(nextFn, ++iteration));
      }
      else {
        executeSoon(nextFn);
      }
    });
  }

  

  setupTestsuite();
  if (navigator.platform.match(/Mac/)) {
    
    testMacNotifications(function () {
      testNotificationCount(function () {
        cleanupTestsuite();
        browserWindowsCount(1, "Only one browser window should be open eventually");
        finish();
      });
    });
  }
  else {
    
    testOpenCloseNormal(function () {
      browserWindowsCount([0, 1], "browser windows after testOpenCloseNormal");
      testOpenClosePrivateBrowsing(function () {
        browserWindowsCount([0, 1], "browser windows after testOpenClosePrivateBrowsing");
        testOpenCloseWindowAndPopup(function () {
          browserWindowsCount([0, 1], "browser windows after testOpenCloseWindowAndPopup");
          testOpenCloseOnlyPopup(function () {
            browserWindowsCount([0, 1], "browser windows after testOpenCloseOnlyPopup");
            testOpenCloseRestoreFromPopup(function () {
              browserWindowsCount([0, 1], "browser windows after testOpenCloseRestoreFromPopup");
              testNotificationCount(function () {
                cleanupTestsuite();
                browserWindowsCount(1, "browser windows after testNotificationCount");
                finish();
              });
            });
          });
        });
      });
    });
  }
}
