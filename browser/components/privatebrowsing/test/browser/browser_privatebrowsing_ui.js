







































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
  let pbMenuItem = document.getElementById("privateBrowsingItem");
  
  let blankTab = gBrowser.addTab();
  gBrowser.selectedTab = blankTab;
  let originalTitle = document.title;
  let privateBrowsingTitle = document.documentElement.getAttribute("titlemodifier_privatebrowsing");

  
  ok(gPrivateBrowsingUI, "The gPrivateBrowsingUI object exists");
  ok(pbMenuItem, "The Private Browsing menu item exists");
  ok(!pbMenuItem.hasAttribute("checked"), "The Private Browsing menu item is not checked initially");
  gPrivateBrowsingUI.toggleMode();
  
  is(observer.data, "enter", "Private Browsing mode was activated using the gPrivateBrowsingUI object");
  ok(pbMenuItem.hasAttribute("checked"), "The Private Browsing menu item was correctly checked");
  gPrivateBrowsingUI.toggleMode()
  
  is(observer.data, "exit", "Private Browsing mode was deactivated using the gPrivateBrowsingUI object");
  ok(!pbMenuItem.hasAttribute("checked"), "The Private Browsing menu item was correctly unchecked");

  
  let cmd = document.getElementById("Tools:PrivateBrowsing");
  isnot(cmd, null, "XUL command object for the private browsing service exists");
  var func = new Function("", cmd.getAttribute("oncommand"));
  func.call(cmd);
  
  is(observer.data, "enter", "Private Browsing mode was activated using the command object");
  
  is(document.title, privateBrowsingTitle, "Private browsing mode has correctly changed the title");
  func.call(cmd);
  
  is(observer.data, "exit", "Private Browsing mode was deactivated using the command object");
  
  is(document.title, originalTitle, "Private browsing mode has correctly restored the title");

  
  gBrowser.removeTab(blankTab);
  os.removeObserver(observer, "private-browsing");
  prefBranch.clearUserPref("browser.privatebrowsing.keep_current_session");
}
