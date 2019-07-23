






































function test() {
  

  waitForExplicitFinish();

  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);

  
  let profilePath = Cc["@mozilla.org/file/directory_service;1"].
                    getService(Ci.nsIProperties).
                    get("ProfD", Ci.nsIFile);
  let sessionStoreJS = profilePath.clone();
  sessionStoreJS.append("sessionstore.js");
  if (sessionStoreJS.exists())
    sessionStoreJS.remove(false);
  ok(sessionStoreJS.exists() == false, "sessionstore.js was removed");
  
  
  gPrefService.setIntPref("browser.sessionstore.interval", 0);
  
  sessionStoreJS = profilePath.clone();
  sessionStoreJS.append("sessionstore.js");

  
  
  let blankState = JSON.stringify({
    windows: [{
      tabs: [{ entries: [{ url: "about:blank" }] }],
      _closedTabs: []
    }],
    _closedWindows: []
  });
  ss.setBrowserState(blankState);

  waitForSessionStoreJS();
}

let pass = 0;
const MAX_PASS = 6;
function waitForSessionStoreJS() {
  if (++pass > MAX_PASS) {
    throw("Timed out waiting for sessionstore.js");
    finish();
  }

  let profilePath = Cc["@mozilla.org/file/directory_service;1"].
                    getService(Ci.nsIProperties).
                    get("ProfD", Ci.nsIFile);
  let sessionStoreJS = profilePath.clone();
  sessionStoreJS.append("sessionstore.js");
  if (sessionStoreJS.exists())
    executeSoon(continue_test);
  else
    setTimeout(500, waitForSessionStoreJS);
}

function continue_test() {
  let ww = Cc["@mozilla.org/embedcomp/window-watcher;1"].
           getService(Ci.nsIWindowWatcher);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);

  let closedWindowCount = ss.getClosedWindowCount();
  is(closedWindowCount, 0, "Correctly set window count");

  gPrefService.clearUserPref("browser.sessionstore.interval");

  
  let now = Date.now();
  const TESTS = [
    { url: "about:config",
      key: "bug 394759 Non-PB",
      value: "uniq" + (++now) },
    { url: "about:mozilla",
      key: "bug 394759 PB",
      value: "uniq" + (++now) },
  ];

  function openWindowAndTest(aTestIndex, aRunNextTestInPBMode) {
    info("Opening new window");
    let windowObserver = {
      observe: function(aSubject, aTopic, aData) {
        if (aTopic === "domwindowopened") {
          info("New window has been opened");
          let win = aSubject.QueryInterface(Ci.nsIDOMWindow);
          is(win.document.readyState, "uninitialized");
          win.addEventListener("load", function onLoad(event) {
            win.removeEventListener("load", onLoad, false);
            info("New window has been loaded");
            win.gBrowser.addEventListener("load", function(aEvent) {
              win.gBrowser.removeEventListener("load", arguments.callee, true);
              info("New window browser has been loaded");
              executeSoon(function() {
                
                win.gBrowser.addTab();

                executeSoon(function() {
                  
                  ss.setWindowValue(win, TESTS[aTestIndex].key, TESTS[aTestIndex].value);

                  win.close();

                  
                  is(ss.getClosedWindowCount(), closedWindowCount + 1,
                     "The closed window was added to the list");

                  
                  let data = JSON.parse(ss.getClosedWindowData())[0];
                  ok(data.toSource().indexOf(TESTS[aTestIndex].value) > -1,
                     "The closed window data was stored correctly");

                  if (aRunNextTestInPBMode) {
                    
                    pb.privateBrowsingEnabled = true;
                    ok(pb.privateBrowsingEnabled, "private browsing enabled");

                    
                    is(ss.getClosedWindowCount(), 0,
                       "Recently Closed Windows are removed when entering Private Browsing");
                    is(ss.getClosedWindowData(), "[]",
                       "Recently Closed Windows data is cleared when entering Private Browsing");
                  }
                  else {
                    
                    pb.privateBrowsingEnabled = false;
                    ok(!pb.privateBrowsingEnabled, "private browsing disabled");

                    
                    is(ss.getClosedWindowCount(), closedWindowCount + 1,
                       "The correct number of recently closed windows were restored " +
                       "when exiting PB mode");

                    let data = JSON.parse(ss.getClosedWindowData())[0];
                    ok(data.toSource().indexOf(TESTS[aTestIndex - 1].value) > -1,
                       "The data associated with the recently closed window was " +
                       "restored when exiting PB mode");
                  }

                  if (aTestIndex == TESTS.length - 1)
                    finish();
                  else {
                    
                    openWindowAndTest(aTestIndex + 1, !aRunNextTestInPBMode);
                  }
                });
              });
            }, true);
          }, false);
          
          let els = Cc["@mozilla.org/eventlistenerservice;1"].
                    getService(Ci.nsIEventListenerService);
          let infos = els.getListenerInfoFor(win, {});
          is(infos.length, 1, "Window has 1 listener");
          is(infos[0].type, "load", "Window has load listener");
          ok(!infos[0].capturing, "Window does not have a capture listener");
        }
        else if (aTopic === "domwindowclosed") {
          info("Window closed");
          ww.unregisterNotification(this);
        }
      }
    };
    ww.registerNotification(windowObserver);
    
    let newWin = openDialog(location, "_blank", "chrome,all,dialog=no", TESTS[aTestIndex].url);
    is(newWin.document.readyState, "uninitialized");
  }

  openWindowAndTest(0, true);
}
