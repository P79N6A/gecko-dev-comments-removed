




































function test() {
  

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  waitForExplicitFinish();

  function test_basic(callback) {
    const PREF_MAX_WIN_UNDO = "browser.sessionstore.max_windows_undo";
    const TEST_URL = "about:config";
    const UNIQUE_KEY = "bug 394759";
    const UNIQUE_VALUE = "unik" + Date.now();
    const UNIQUE_TEXT = "pi != " + Math.random();

    
    let max_windows_undo = gPrefService.getIntPref(PREF_MAX_WIN_UNDO);
    gPrefService.setIntPref(PREF_MAX_WIN_UNDO, max_windows_undo + 1);
    let closedWindowCount = ss.getClosedWindowCount();

    let newWin = openDialog(location, "_blank", "chrome,all,dialog=no", TEST_URL);
    newWin.addEventListener("load", function(aEvent) {
      newWin.removeEventListener("load", arguments.callee, false);
      newWin.gBrowser.addEventListener("load", function(aEvent) {
        newWin.gBrowser.removeEventListener("load", arguments.callee, true);
        executeSoon(function() {
          newWin.gBrowser.addTab().linkedBrowser.stop();

          executeSoon(function() {
            
            ss.setWindowValue(newWin, UNIQUE_KEY, UNIQUE_VALUE);
            let textbox = newWin.content.document.getElementById("textbox");
            textbox.wrappedJSObject.value = UNIQUE_TEXT;

            newWin.close();

            is(ss.getClosedWindowCount(), closedWindowCount + 1,
               "The closed window was added to Recently Closed Windows");
            let data = JSON.parse(ss.getClosedWindowData())[0];
            ok(data.title == TEST_URL &&
               data.toSource().indexOf(UNIQUE_TEXT) > -1,
               "The closed window data was stored correctly");

            
            let newWin2 = ss.undoCloseWindow(0);

            ok(newWin2 instanceof ChromeWindow,
               "undoCloseWindow actually returned a window");
            is(ss.getClosedWindowCount(), closedWindowCount,
               "The reopened window was removed from Recently Closed Windows");

            newWin2.addEventListener("load", function(aEvent) {
              newWin2.removeEventListener("load", arguments.callee, false);
              newWin2.gBrowser.addEventListener("SSTabRestored", function(aEvent) {
                newWin2.gBrowser.removeEventListener("SSTabRestored", arguments.callee, true);
                executeSoon(function() {
                  is(newWin2.gBrowser.tabContainer.childNodes.length, 2,
                     "The window correctly restored 2 tabs");
                  is(newWin2.gBrowser.currentURI.spec, TEST_URL,
                     "The window correctly restored the URL");

                  let textbox = newWin2.content.document.getElementById("textbox");
                  is(textbox.wrappedJSObject.value, UNIQUE_TEXT,
                     "The window correctly restored the form");
                  is(ss.getWindowValue(newWin2, UNIQUE_KEY), UNIQUE_VALUE,
                     "The window correctly restored the data associated with it");

                  newWin2.close();

                  if (gPrefService.prefHasUserValue(PREF_MAX_WIN_UNDO))
                    gPrefService.clearUserPref(PREF_MAX_WIN_UNDO);

                  executeSoon(callback);
                });
              }, true);
            }, false);
          });
        });
      }, true);
    }, false);
  }
  
  function test_behavior (callback) {
    
    function openWindowRec(windowsToOpen, expectedResults, recCallback) {
      
      if (!windowsToOpen.length) {
        let closedWindowData = JSON.parse(ss.getClosedWindowData());
        let numPopups = closedWindowData.filter(function(el, i, arr) {
          return el.isPopup;
        }).length;
        let numNormal = ss.getClosedWindowCount() - numPopups;
        
        let oResults = navigator.platform.match(/Mac/) ? expectedResults.mac
                                                       : expectedResults.other;
        is(numPopups, oResults.popup,
           "There were " + oResults.popup + " popup windows to repoen");
        is(numNormal, oResults.normal,
           "There were " + oResults.normal + " normal windows to repoen");

        
        executeSoon(recCallback);
        return;
      }

      
      let winData = windowsToOpen.shift();
      let settings = "chrome,dialog=no," +
                     (winData.isPopup ? "all=no" : "all");
      let url = "http://window" + windowsToOpen.length + ".example.com";
      let newWin = openDialog(location, "_blank", settings, url);
      newWin.addEventListener("load", function(aEvent) {
        newWin.removeEventListener("load", arguments.callee, false);
        newWin.gBrowser.addEventListener("load", function(aEvent) {
          newWin.gBrowser.removeEventListener("load", arguments.callee, true);
          executeSoon(function() {
            
            
            
            newWin.gBrowser.addTab().linkedBrowser.stop();
            executeSoon(function() {
              newWin.close();

              openWindowRec(windowsToOpen, expectedResults, recCallback);
            });
          });
        }, true);
      }, false);
    }

    let windowsToOpen = [{isPopup: false},
                         {isPopup: false},
                         {isPopup: true},
                         {isPopup: true},
                         {isPopup: true}];
    let expectedResults = {mac: {popup: 3, normal: 0},
                           other: {popup: 3, normal: 1}};
    let windowsToOpen2 = [{isPopup: false},
                          {isPopup: false},
                          {isPopup: false},
                          {isPopup: false},
                          {isPopup: false}];
    let expectedResults2 = {mac: {popup: 0, normal: 3},
                            other: {popup: 0, normal: 3}};
    openWindowRec(windowsToOpen, expectedResults, function() {
      openWindowRec(windowsToOpen2, expectedResults2, callback);
    });
  }

  function test_purge(callback) {
    
    function countClosedTabsByTitle(aClosedTabList, aTitle)
      aClosedTabList.filter(function(aData) aData.title == aTitle).length;

    function countOpenTabsByTitle(aOpenTabList, aTitle)
      aOpenTabList.filter(function(aData) aData.entries.some(function(aEntry) aEntry.title == aTitle) ).length

    
    const REMEMBER = Date.now(), FORGET = Math.random();
    let testState = {
      windows: [ { tabs: [{ entries: [{ url: "http://example.com/" }] }], selected: 1 } ],
      _closedWindows : [
        
        {
          tabs: [
            { entries: [{ url: "http://example.com/", title: REMEMBER }] },
            { entries: [{ url: "http://mozilla.org/", title: FORGET }] }
          ],
          selected: 2,
          title: "mozilla.org",
          _closedTabs: []
        },
        
        {
          tabs: [
           { entries: [{ url: "http://mozilla.org/", title: FORGET }] },
           { entries: [{ url: "http://example.com/", title: REMEMBER }] },
           { entries: [{ url: "http://example.com/", title: REMEMBER }] },
           { entries: [{ url: "http://mozilla.org/", title: FORGET }] },
           { entries: [{ url: "http://example.com/", title: REMEMBER }] }
          ],
          selected: 5,
          _closedTabs: []
        },
        
        {
          tabs: [
            { entries: [{ url: "http://example.com/", title: REMEMBER }] }
          ],
          selected: 1,
          _closedTabs: [
            {
              state: {
                entries: [
                  { url: "http://mozilla.org/", title: FORGET },
                  { url: "http://mozilla.org/again", title: "doesn't matter" }
                ]
              },
              pos: 1,
              title: FORGET
            },
            {
              state: {
                entries: [
                  { url: "http://example.com", title: REMEMBER }
                ]
              },
              title: REMEMBER
            }
          ]
        }
      ]
    };

    
    ss.setBrowserState(JSON.stringify(testState));

    
    pb.removeDataFromDomain("mozilla.org");

    let closedWindowData = JSON.parse(ss.getClosedWindowData());

    
    let win = closedWindowData[0];
    is(win.tabs.length, 1, "1 tab was removed");
    is(countOpenTabsByTitle(win.tabs, FORGET), 0,
       "The correct tab was removed");
    is(countOpenTabsByTitle(win.tabs, REMEMBER), 1,
       "The correct tab was remembered");
    is(win.selected, 1, "Selected tab has changed");
    is(win.title, REMEMBER, "The window title was correctly updated");

    
    win = closedWindowData[1];
    is(win.tabs.length, 3, "2 tabs were removed");
    is(countOpenTabsByTitle(win.tabs, FORGET), 0,
       "The correct tabs were removed");
    is(countOpenTabsByTitle(win.tabs, REMEMBER), 3,
       "The correct tabs were remembered");
    is(win.selected, 3, "Selected tab has changed");
    is(win.title, REMEMBER, "The window title was correctly updated");

    
    win = closedWindowData[2];
    is(countClosedTabsByTitle(win._closedTabs, REMEMBER), 1,
       "The correct number of tabs were removed, and the correct ones");
    is(countClosedTabsByTitle(win._closedTabs, FORGET), 0,
       "All tabs to be forgotten were indeed removed");

    
    let blankState = JSON.stringify({
      windows: [{
        tabs: [{ entries: [{ url: "about:blank" }] }],
        _closedTabs: []
      }],
      _closedWindows: []
    });
    ss.setBrowserState(blankState);

    executeSoon(callback);
  }

  test_basic(function() {
    test_behavior(function() {
      test_purge(finish);
    });
  });
}
