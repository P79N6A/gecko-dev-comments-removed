




































function test() {
  

  
  waitForExplicitFinish();

  
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let profilePath = Cc["@mozilla.org/file/directory_service;1"].
                    getService(Ci.nsIProperties).
                    get("ProfD", Ci.nsIFile);

  function getSessionstorejsModificationTime() {
    
    let file = Cc["@mozilla.org/file/directory_service;1"].
               getService(Ci.nsIProperties).
               get("ProfD", Ci.nsIFile);

    
    file.append("sessionstore.js");

    if (file.exists())
      return file.lastModifiedTime;
    else
      return -1;
  }

  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].
           getService(Ci.nsISessionStore);
  
  let sessionStoreJS = profilePath.clone();
  sessionStoreJS.append("sessionstore.js");
  if (sessionStoreJS.exists())
    sessionStoreJS.remove(false);
  
  
  gPrefService.setIntPref("browser.sessionstore.interval", 0);
  
  sessionStoreJS = profilePath.clone();
  sessionStoreJS.append("sessionstore.js");
  ok(sessionStoreJS.exists(),
    "sessionstore.js should be re-created after setting the interval to 0");

  
  
  

  
  const testURL_A = "http://example.org/";
  let tab_A = gBrowser.addTab(testURL_A);

  tab_A.linkedBrowser.addEventListener("load", function (aEvent) {
    this.removeEventListener("load", arguments.callee, true);

    
    
    let sessionStoreJS = profilePath.clone();
    sessionStoreJS.append("sessionstore.js");
    ok(sessionStoreJS.exists(),
      "sessionstore.js should exist prior to entering the private browsing mode");
    sessionStoreJS.remove(false);

    
    pb.privateBrowsingEnabled = true;
    ok(pb.privateBrowsingEnabled, "private browsing enabled");

    
    sessionStoreJS = profilePath.clone();
    sessionStoreJS.append("sessionstore.js");
    ok(sessionStoreJS.exists(),
      "sessionstore.js should be re-created after entering the private browsing mode");

    
    let startPBModeTimeStamp = getSessionstorejsModificationTime();

    
    const testURL_B = "http://test1.example.org/";
    let tab_B = gBrowser.addTab(testURL_B);

    tab_B.linkedBrowser.addEventListener("load", function (aEvent) {
      this.removeEventListener("load", arguments.callee, true);

      
      const testURL_C = "http://localhost:8888/";
      let tab_C = gBrowser.addTab(testURL_C);

      tab_C.linkedBrowser.addEventListener("load", function (aEvent) {
        this.removeEventListener("load", arguments.callee, true);

        
        gBrowser.removeTab(tab_C);

        
        gBrowser.removeTab(tab_B);

        
        gBrowser.removeTab(tab_A);

        
        gPrefService.clearUserPref("browser.sessionstore.interval");
        gPrefService.setIntPref("browser.sessionstore.interval", 0);
        let endPBModeTimeStamp = getSessionstorejsModificationTime();

        
        pb.privateBrowsingEnabled = false;
        ok(!pb.privateBrowsingEnabled, "private browsing disabled");

        
        is(startPBModeTimeStamp, endPBModeTimeStamp,
          "outside private browsing - sessionStore.js timestamp has not changed");

        
        gPrefService.clearUserPref("browser.sessionstore.interval");
        gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
        finish();
      }, true);
    }, true);
  }, true);
}
