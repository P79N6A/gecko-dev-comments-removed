



































function test() {
  
  
  
  let ss = Cc["@mozilla.org/browser/sessionstore;1"].getService(Ci.nsISessionStore);
  let os = Cc["@mozilla.org/observer-service;1"].getService(Ci.nsIObserverService);
  let tabbrowser = getBrowser();
  waitForExplicitFinish();
  
  let uniqueName = "bug 448741";
  let uniqueValue = "as good as unique: " + Date.now();
  let interval = gPrefService.getIntPref("browser.sessionstore.interval");
  
  
  var tab = tabbrowser.addTab();
  ss.setTabValue(tab, uniqueName, uniqueValue);
  let valueWasCleaned = false;
  
  
  let cleaningObserver = {
    observe: function(aSubject, aTopic, aData) {
      ok(aTopic == "sessionstore-state-write", "observed correct topic?");
      ok(aSubject instanceof Ci.nsISupportsString, "subject is a string?");
      ok(aSubject.data.indexOf(uniqueValue) > -1, "data contains our value?");
      
      
      let state = eval(aSubject.data);
      for each (let winData in state.windows) {
        for each (let tabData in winData.tabs) {
          if (tabData.extData && uniqueName in tabData.extData &&
              tabData.extData[uniqueName] == uniqueValue) {
            delete tabData.extData[uniqueName];
            valueWasCleaned = true;
          }
        }
      }
      
      ok(valueWasCleaned, "found and removed the specific tab value");
      aSubject.data = uneval(state);
      os.removeObserver(this, aTopic, false);
    }
  };
  
  
  let checkingObserver = {
    observe: function(aSubject, aTopic, aData) {
      ok(valueWasCleaned && aSubject instanceof Ci.nsISupportsString,
         "ready to check the cleaned state?");
      ok(aSubject.data.indexOf(uniqueValue) == -1, "data no longer contains our value?");
      
      
      tabbrowser.removeTab(tab);
      os.removeObserver(this, aTopic, false);
      gPrefService.setIntPref("browser.sessionstore.interval", interval);
      finish();
    }
  };
  
  
  os.addObserver(checkingObserver, "sessionstore-state-write", false);
  os.addObserver(cleaningObserver, "sessionstore-state-write", false);
  
  
  gPrefService.setIntPref("browser.sessionstore.interval", 0);
}
