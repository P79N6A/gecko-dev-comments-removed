







































function test() {
  
  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  
  pb.privateBrowsingEnabled = true;

  let tabAbout = gBrowser.addTab();
  gBrowser.selectedTab = tabAbout;

  waitForExplicitFinish();

  let aboutBrowser = gBrowser.getBrowserForTab(tabAbout);
  aboutBrowser.addEventListener("load", function () {
    aboutBrowser.removeEventListener("load", arguments.callee, true);
    let tabRobots = gBrowser.addTab();
    gBrowser.selectedTab = tabRobots;

    let robotsBrowser = gBrowser.getBrowserForTab(tabRobots);
    robotsBrowser.addEventListener("load", function () {
      robotsBrowser.removeEventListener("load", arguments.callee, true);
      let robotsZoom = ZoomManager.zoom;

      
      FullZoom.enlarge();
      
      isnot(ZoomManager.zoom, robotsZoom, "Zoom level can be changed");
      robotsZoom = ZoomManager.zoom;

      
      gBrowser.selectedTab = tabAbout;

      
      gBrowser.selectedTab = tabRobots;

      
      is(ZoomManager.zoom, robotsZoom,
        "Entering private browsing should not reset the zoom on a tab");

      
      pb.privateBrowsingEnabled = false;

      
      gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
      FullZoom.reset();
      gBrowser.removeTab(tabRobots);
      gBrowser.removeTab(tabAbout);
      finish();
    }, true);
    robotsBrowser.contentWindow.location = "about:robots";
  }, true);
  aboutBrowser.contentWindow.location = "about:";
}
