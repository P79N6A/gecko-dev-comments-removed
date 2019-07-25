




































function test() {
  

  
  waitForExplicitFinish();

  
  let profilePath = Cc["@mozilla.org/file/directory_service;1"].
                    getService(Ci.nsIProperties).
                    get("ProfD", Ci.nsIFile);
  function getSessionstoreFile() {
    let sessionStoreJS = profilePath.clone();
    sessionStoreJS.append("sessionstore.js");
    return sessionStoreJS;
  }
  function getSessionstorejsModificationTime() {
    let file = getSessionstoreFile();
    if (file.exists())
      return file.lastModifiedTime;
    else
      return -1;
  }

  
  
  let (sessionStoreJS = getSessionstoreFile()) {
    if (sessionStoreJS.exists())
      sessionStoreJS.remove(false);
  }

  
  const TEST_URL = "data:text/html,"
    + "<body style='width: 100000px; height: 100000px;'><p>top</p></body>"

  
  const PREF_INTERVAL = "browser.sessionstore.interval";

  
  gPrefService.setIntPref(PREF_INTERVAL, 0);

  
  let mtime0 = getSessionstorejsModificationTime();

  
  let tab = gBrowser.addTab(TEST_URL);
  tab.linkedBrowser.addEventListener("load", function loadListener(e) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    
    
    setTimeout(function step1(e) {
      let mtime1 = getSessionstorejsModificationTime();
      isnot(mtime1, mtime0, "initial sessionstore.js update");

      
      
      gBrowser.selectedTab = tab;
      tab.linkedBrowser.contentWindow.scrollTo(1100, 1200);
      setTimeout(function step2(e) {
        let mtime2 = getSessionstorejsModificationTime();
        is(mtime2, mtime1, 
           "tab selection and scrolling: sessionstore.js not updated");

        
        if (gPrefService.prefHasUserValue(PREF_INTERVAL))
          gPrefService.clearUserPref(PREF_INTERVAL);
        gBrowser.removeTab(tab);
        finish();
      }, 3500); 
    }, 3500); 
  }, true);
}
