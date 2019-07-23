






































function test() {
  
  let prefBranch = Cc["@mozilla.org/preferences-service;1"].
                   getService(Ci.nsIPrefBranch);
  prefBranch.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let observer = {
    observe: function (aSubject, aTopic, aData) {
      if (aTopic == "private-browsing")
        this.data = aData;
    },
    data: null
  };
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.addObserver(observer, "private-browsing", false);

  
  ok(gPrivateBrowsingUI, "The gPrivateBrowsingUI object exists");
  gPrivateBrowsingUI.toggleMode();
  
  is(observer.data, "enter", "Private Browsing mode was activated using the gPrivateBrowsingUI object");
  gPrivateBrowsingUI.toggleMode()
  
  is(observer.data, "exit", "Private Browsing mode was deactivated using the gPrivateBrowsingUI object");
}
