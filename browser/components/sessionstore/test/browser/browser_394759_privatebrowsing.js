





































function test() {
  

  
  waitForExplicitFinish();

  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let profilePath = Cc["@mozilla.org/file/directory_service;1"].
                    getService(Ci.nsIProperties).
                    get("ProfD", Ci.nsIFile);

  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  
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

  let closedWindowCount = ss.getClosedWindowCount();
  is(closedWindowCount, 0, "Correctly set window count");

  let testURL_A = "about:config";
  let testURL_B = "about:mozilla";

  let uniqueKey_A = "bug 394759 Non-PB";
  let uniqueValue_A = "unik" + Date.now();
  let uniqueKey_B = "bug 394759 PB";
  let uniqueValue_B = "uniq" + Date.now();


  
  let newWin = openDialog(location, "_blank", "chrome,all,dialog=no", testURL_A);
  newWin.addEventListener("load", function(aEvent) {
    newWin.removeEventListener("load", arguments.callee, false);
    newWin.gBrowser.addEventListener("load", function(aEvent) {
      newWin.gBrowser.removeEventListener("load", arguments.callee, true);
      info("Window has been loaded");
      executeSoon(function() {
        newWin.gBrowser.addTab();
        executeSoon(function() {
          
          ss.setWindowValue(newWin, uniqueKey_A, uniqueValue_A);

          newWin.close();

          
          is(ss.getClosedWindowCount(), closedWindowCount + 1,
             "The closed window was added to the list");

          
          let data = JSON.parse(ss.getClosedWindowData())[0];
          ok(data.toSource().indexOf(uniqueValue_A) > -1,
             "The closed window data was stored correctly");

          
          pb.privateBrowsingEnabled = true;
          ok(pb.privateBrowsingEnabled, "private browsing enabled");

          
          is(ss.getClosedWindowCount(), 0,
             "Recently Closed Windows are removed when entering Private Browsing");
          is(ss.getClosedWindowData(), "[]",
             "Recently Closed Windows data is cleared when entering Private Browsing");

          
          let pbWin = openDialog(location, "_blank", "chrome,all,dialog=no", testURL_B);
          pbWin.addEventListener("load", function(aEvent) {
            pbWin.removeEventListener("load", arguments.callee, false);
            pbWin.gBrowser.addEventListener("load", function(aEvent) {
              pbWin.gBrowser.removeEventListener("load", arguments.callee, true);

              executeSoon(function() {
                
                pbWin.gBrowser.addTab();
                executeSoon(function() {
                  
                  ss.setWindowValue(pbWin, uniqueKey_B, uniqueValue_B);

                  pbWin.close();

                  
                  let data = JSON.parse(ss.getClosedWindowData())[0];
                  ok(data.toSource().indexOf(uniqueValue_B) > -1,
                     "The closed window data was stored correctly in PB mode");

                  
                  pb.privateBrowsingEnabled = false;
                  ok(!pb.privateBrowsingEnabled, "private browsing disabled");

                  
                  is(ss.getClosedWindowCount(), closedWindowCount + 1,
                     "The correct number of recently closed windows were restored " +
                     "when exiting PB mode");

                  let data = JSON.parse(ss.getClosedWindowData())[0];
                  ok(data.toSource().indexOf(uniqueValue_A) > -1,
                     "The data associated with the recently closed window was " +
                     "restored when exiting PB mode");

                  
                  gPrefService.clearUserPref("browser.sessionstore.interval");
                  finish();
                });
              });
            }, true);
          }, false);
        });
      });
    }, true);
  }, false);
}
