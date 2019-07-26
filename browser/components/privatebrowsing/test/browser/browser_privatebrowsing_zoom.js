






function test() {
  waitForExplicitFinish();

  function testZoom(aWindow, aCallback) {
    executeSoon(function() {
      let tabAbout = aWindow.gBrowser.addTab();
      aWindow.gBrowser.selectedTab = tabAbout;

      let aboutBrowser = aWindow.gBrowser.getBrowserForTab(tabAbout);
      aboutBrowser.addEventListener("load", function onAboutBrowserLoad() {
        aboutBrowser.removeEventListener("load", onAboutBrowserLoad, true);
        let tabMozilla = aWindow.gBrowser.addTab();
        aWindow.gBrowser.selectedTab = tabMozilla;

        let mozillaBrowser = aWindow.gBrowser.getBrowserForTab(tabMozilla);
        mozillaBrowser.addEventListener("load", function onMozillaBrowserLoad() {
          mozillaBrowser.removeEventListener("load", onMozillaBrowserLoad, true);
          let mozillaZoom = aWindow.ZoomManager.zoom;

          
          aWindow.FullZoom.enlarge();
          
          isnot(aWindow.ZoomManager.zoom, mozillaZoom, "Zoom level can be changed");
          mozillaZoom = aWindow.ZoomManager.zoom;

          
          aWindow.gBrowser.selectedTab = tabAbout;

          
          aWindow.gBrowser.selectedTab = tabMozilla;

          
          is(aWindow.ZoomManager.zoom, mozillaZoom,
            "Entering private browsing should not reset the zoom on a tab");

          
          aWindow.FullZoom.reset();
          aWindow.gBrowser.removeTab(tabMozilla);
          aWindow.gBrowser.removeTab(tabAbout);
          aWindow.close();
          aCallback();
        }, true);
        mozillaBrowser.contentWindow.location = "about:mozilla";
      }, true);
      aboutBrowser.contentWindow.location = "about:";
    });
  }

  whenNewWindowLoaded({private: true}, function(privateWindow) {
    testZoom(privateWindow, finish);
  });
}
