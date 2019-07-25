






function test() {
  
  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);

  
  pb.privateBrowsingEnabled = true;

  let tabAbout = gBrowser.addTab();
  gBrowser.selectedTab = tabAbout;

  waitForExplicitFinish();

  let aboutBrowser = gBrowser.getBrowserForTab(tabAbout);
  aboutBrowser.addEventListener("load", function onAboutBrowserLoad() {
    aboutBrowser.removeEventListener("load", onAboutBrowserLoad, true);
    let tabMozilla = gBrowser.addTab();
    gBrowser.selectedTab = tabMozilla;

    let mozillaBrowser = gBrowser.getBrowserForTab(tabMozilla);
    mozillaBrowser.addEventListener("load", function onMozillaBrowserLoad() {
      mozillaBrowser.removeEventListener("load", onMozillaBrowserLoad, true);
      let mozillaZoom = ZoomManager.zoom;

      
      FullZoom.enlarge();
      
      isnot(ZoomManager.zoom, mozillaZoom, "Zoom level can be changed");
      mozillaZoom = ZoomManager.zoom;

      
      gBrowser.selectedTab = tabAbout;

      
      gBrowser.selectedTab = tabMozilla;

      
      is(ZoomManager.zoom, mozillaZoom,
        "Entering private browsing should not reset the zoom on a tab");

      
      pb.privateBrowsingEnabled = false;

      
      gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
      FullZoom.reset();
      gBrowser.removeTab(tabMozilla);
      gBrowser.removeTab(tabAbout);
      finish();
    }, true);
    mozillaBrowser.contentWindow.location = "about:mozilla";
  }, true);
  aboutBrowser.contentWindow.location = "about:";
}
