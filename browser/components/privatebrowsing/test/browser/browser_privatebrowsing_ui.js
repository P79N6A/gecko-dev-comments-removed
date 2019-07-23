







































function test() {
  
  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  let observerData;
  function observer(aSubject, aTopic, aData) {
    if (aTopic == "private-browsing")
      observerData = aData;
  }
  Services.obs.addObserver(observer, "private-browsing", false);
  let pbMenuItem = document.getElementById("privateBrowsingItem");
  
  gBrowser.selectedTab = gBrowser.addTab();
  let originalTitle = document.title;

  
  ok(gPrivateBrowsingUI, "The gPrivateBrowsingUI object exists");
  is(pb.privateBrowsingEnabled, false, "The private browsing mode should not be started initially");
  is(gPrivateBrowsingUI.privateBrowsingEnabled, false, "gPrivateBrowsingUI should expose the correct private browsing status");
  ok(pbMenuItem, "The Private Browsing menu item exists");
  is(pbMenuItem.getAttribute("label"), pbMenuItem.getAttribute("startlabel"), "The Private Browsing menu item should read \"Start Private Browsing\"");
  gPrivateBrowsingUI.toggleMode();
  is(pb.privateBrowsingEnabled, true, "The private browsing mode should be started");
  is(gPrivateBrowsingUI.privateBrowsingEnabled, true, "gPrivateBrowsingUI should expose the correct private browsing status");
  
  is(observerData, "enter", "Private Browsing mode was activated using the gPrivateBrowsingUI object");
  is(pbMenuItem.getAttribute("label"), pbMenuItem.getAttribute("stoplabel"), "The Private Browsing menu item should read \"Stop Private Browsing\"");
  gPrivateBrowsingUI.toggleMode()
  is(pb.privateBrowsingEnabled, false, "The private browsing mode should not be started");
  is(gPrivateBrowsingUI.privateBrowsingEnabled, false, "gPrivateBrowsingUI should expose the correct private browsing status");
  
  is(observerData, "exit", "Private Browsing mode was deactivated using the gPrivateBrowsingUI object");
  is(pbMenuItem.getAttribute("label"), pbMenuItem.getAttribute("startlabel"), "The Private Browsing menu item should read \"Start Private Browsing\"");

  
  let cmd = document.getElementById("Tools:PrivateBrowsing");
  isnot(cmd, null, "XUL command object for the private browsing service exists");
  var func = new Function("", cmd.getAttribute("oncommand"));
  func.call(cmd);
  
  is(observerData, "enter", "Private Browsing mode was activated using the command object");
  
  isnot(document.title, originalTitle, "Private browsing mode has correctly changed the title");
  func.call(cmd);
  
  is(observerData, "exit", "Private Browsing mode was deactivated using the command object");
  
  is(document.title, originalTitle, "Private browsing mode has correctly restored the title");

  
  gBrowser.removeCurrentTab();
  Services.obs.removeObserver(observer, "private-browsing");
  gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
}
