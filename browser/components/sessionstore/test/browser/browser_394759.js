




































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
  
  
  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].getService(Ci.nsIPrivateBrowsingService);
  waitForExplicitFinish();
  
  function test_basic(callback) {
  
    let testURL = "about:config";
    let uniqueKey = "bug 394759";
    let uniqueValue = "unik" + Date.now();
    let uniqueText = "pi != " + Math.random();
  
  
    
    let max_windows_undo = gPrefService.getIntPref("browser.sessionstore.max_windows_undo");
    gPrefService.setIntPref("browser.sessionstore.max_windows_undo", max_windows_undo + 1);
    let closedWindowCount = ss.getClosedWindowCount();
  
    let newWin = openDialog(location, "_blank", "chrome,all,dialog=no", testURL);
    newWin.addEventListener("load", function(aEvent) {
      newWin.removeEventListener("load", arguments.callee, false);
      newWin.gBrowser.addEventListener("load", function(aEvent) {
        newWin.gBrowser.removeEventListener("load", arguments.callee, true);

        executeSoon(function() {
          newWin.gBrowser.addTab().linkedBrowser.stop();
          executeSoon(function() {
            
            ss.setWindowValue(newWin, uniqueKey, uniqueValue);
            let textbox = newWin.content.document.getElementById("textbox");
            textbox.wrappedJSObject.value = uniqueText;

            newWin.close();

            is(ss.getClosedWindowCount(), closedWindowCount + 1,
               "The closed window was added to Recently Closed Windows");
            let data = JSON.parse(ss.getClosedWindowData())[0];
            ok(data.title == testURL && data.toSource().indexOf(uniqueText) > -1,
               "The closed window data was stored correctly");

            
            let newWin2 = ss.undoCloseWindow(0);

            ok(newWin2 instanceof ChromeWindow,
               "undoCloseWindow actually returned a window");
            is(ss.getClosedWindowCount(), closedWindowCount,
               "The reopened window was removed from Recently Closed Windows");

            newWin2.addEventListener("load", function(aEvent) {
              this.removeEventListener("load", arguments.callee, false);
              newWin2.gBrowser.addEventListener("SSTabRestored", function(aEvent) {
                newWin2.gBrowser.removeEventListener("SSTabRestored", arguments.callee, true);

                is(newWin2.gBrowser.tabContainer.childNodes.length, 2,
                   "The window correctly restored 2 tabs");
                is(newWin2.gBrowser.currentURI.spec, testURL,
                   "The window correctly restored the URL");

                let textbox = newWin2.content.document.getElementById("textbox");
                is(textbox.wrappedJSObject.value, uniqueText,
                   "The window correctly restored the form");
                is(ss.getWindowValue(newWin2, uniqueKey), uniqueValue,
                   "The window correctly restored the data associated with it");

                
                newWin2.close();
                if (gPrefService.prefHasUserValue("browser.sessionstore.max_windows_undo"))
                  gPrefService.clearUserPref("browser.sessionstore.max_windows_undo");
                executeSoon(callback);
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
      let window = openDialog(location, "_blank", settings, url);
      window.addEventListener("load", function(aEvent) {
        this.removeEventListener("load", arguments.callee, true);
        window.gBrowser.addEventListener("load", function(aEvent) {
          this.removeEventListener("load", arguments.callee, true);
          
          
          
          window.gBrowser.addTab();

          executeSoon(function() {
            window.close();
            executeSoon(function() {
              openWindowRec(windowsToOpen, expectedResults, recCallback);
            });
          });
        }, true);
      }, true);
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

    
    let oldState = ss.getBrowserState();
    
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

    
    let window = closedWindowData[0];
    is(window.tabs.length, 1, "1 tab was removed");
    is(countOpenTabsByTitle(window.tabs, FORGET), 0,
       "The correct tab was removed");
    is(countOpenTabsByTitle(window.tabs, REMEMBER), 1,
       "The correct tab was remembered");
    is(window.selected, 1, "Selected tab has changed");
    is(window.title, REMEMBER, "The window title was correctly updated");

    
    window = closedWindowData[1];
    is(window.tabs.length, 3, "2 tabs were removed");
    is(countOpenTabsByTitle(window.tabs, FORGET), 0,
       "The correct tabs were removed");
    is(countOpenTabsByTitle(window.tabs, REMEMBER), 3,
       "The correct tabs were remembered");
    is(window.selected, 3, "Selected tab has changed");
    is(window.title, REMEMBER, "The window title was correctly updated");

    
    window = closedWindowData[2];
    is(countClosedTabsByTitle(window._closedTabs, REMEMBER), 1,
       "The correct number of tabs were removed, and the correct ones");
    is(countClosedTabsByTitle(window._closedTabs, FORGET), 0,
       "All tabs to be forgotten were indeed removed");

    
    ss.setBrowserState(oldState);
    executeSoon(callback);
  }
  
  is(browserWindowsCount(), 1, "Only one browser window should be open initially");
  test_basic(function() {
    is(browserWindowsCount(), 1, "number of browser windows after test_basic");
    test_behavior(function() {
      is(browserWindowsCount(), 1, "number of browser windows after test_behavior");
      test_purge(function() {
        is(browserWindowsCount(), 1, "number of browser windows after test_purge");
        finish();
      });
    });
  });
}
