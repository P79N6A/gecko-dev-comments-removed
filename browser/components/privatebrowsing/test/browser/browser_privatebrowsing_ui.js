







































function test() {
  
  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
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
  
  gBrowser.selectedTab = gBrowser.addTab();
  let originalTitle = document.title;

  
  ok(gPrivateBrowsingUI, "The gPrivateBrowsingUI object exists");
  ok(pbMenuItem, "The Private Browsing menu item exists");
  is(pbMenuItem.getAttribute("label"), pbMenuItem.getAttribute("startlabel"), "The Private Browsing menu item should read \"Start Private Browsing\"");
  gPrivateBrowsingUI.toggleMode();
  
  is(observer.data, "enter", "Private Browsing mode was activated using the gPrivateBrowsingUI object");
  is(pbMenuItem.getAttribute("label"), pbMenuItem.getAttribute("stoplabel"), "The Private Browsing menu item should read \"Stop Private Browsing\"");
  gPrivateBrowsingUI.toggleMode()
  
  is(observer.data, "exit", "Private Browsing mode was deactivated using the gPrivateBrowsingUI object");
  is(pbMenuItem.getAttribute("label"), pbMenuItem.getAttribute("startlabel"), "The Private Browsing menu item should read \"Start Private Browsing\"");

  
  let cmd = document.getElementById("Tools:PrivateBrowsing");
  isnot(cmd, null, "XUL command object for the private browsing service exists");
  var func = new Function("", cmd.getAttribute("oncommand"));
  func.call(cmd);
  
  is(observer.data, "enter", "Private Browsing mode was activated using the command object");
  
  isnot(document.title, originalTitle, "Private browsing mode has correctly changed the title");
  func.call(cmd);
  
  is(observer.data, "exit", "Private Browsing mode was deactivated using the command object");
  
  is(document.title, originalTitle, "Private browsing mode has correctly restored the title");

  
  gBrowser.removeCurrentTab();
  os.removeObserver(observer, "private-browsing");
  gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
}
