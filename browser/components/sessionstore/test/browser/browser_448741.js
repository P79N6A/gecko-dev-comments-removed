



































function test() {
  

  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  waitForExplicitFinish();

  let uniqueName = "bug 448741";
  let uniqueValue = "as good as unique: " + Date.now();

  
  var tab = gBrowser.addTab();
  tab.linkedBrowser.stop();
  ss.setTabValue(tab, uniqueName, uniqueValue);
  let valueWasCleaned = false;

  
  function cleaningObserver(aSubject, aTopic, aData) {
    ok(aTopic == "sessionstore-state-write", "observed correct topic?");
    ok(aSubject instanceof Ci.nsISupportsString, "subject is a string?");
    ok(aSubject.data.indexOf(uniqueValue) > -1, "data contains our value?");

    
    let state = JSON.parse(aSubject.data);
    state.windows.forEach(function (winData) {
      winData.tabs.forEach(function (tabData) {
        if (tabData.extData && uniqueName in tabData.extData &&
            tabData.extData[uniqueName] == uniqueValue) {
          delete tabData.extData[uniqueName];
          valueWasCleaned = true;
        }
      });
    });

    ok(valueWasCleaned, "found and removed the specific tab value");
    aSubject.data = JSON.stringify(state);
    Services.obs.removeObserver(cleaningObserver, aTopic, false);
  }

  
  function checkingObserver(aSubject, aTopic, aData) {
    ok(valueWasCleaned && aSubject instanceof Ci.nsISupportsString,
       "ready to check the cleaned state?");
    ok(aSubject.data.indexOf(uniqueValue) == -1, "data no longer contains our value?");

    
    gBrowser.removeTab(tab);
    Services.obs.removeObserver(checkingObserver, aTopic, false);
    if (gPrefService.prefHasUserValue("browser.sessionstore.interval"))
      gPrefService.clearUserPref("browser.sessionstore.interval");
    finish();
  }

  
  Services.obs.addObserver(checkingObserver, "sessionstore-state-write", false);
  Services.obs.addObserver(cleaningObserver, "sessionstore-state-write", false);

  
  gPrefService.setIntPref("browser.sessionstore.interval", 0);
}
