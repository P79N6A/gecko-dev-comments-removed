function test() {

  
  

  waitForExplicitFinish();

  let oldOLC = FullZoom.onLocationChange;
  FullZoom.onLocationChange = function(aURI, aIsTabSwitch, aBrowser) {
    
    if (aIsTabSwitch)
      oldOLC.call(FullZoom, aURI, aIsTabSwitch, aBrowser);
  };

  gPrefService.setBoolPref("browser.zoom.updateBackgroundTabs", true);
  gPrefService.setBoolPref("browser.zoom.siteSpecific", true);

  let oldAPTS = FullZoom._applyPrefToSetting;
  let uri = "http://example.org/browser/browser/base/content/test/dummy_page.html";

  let tab = gBrowser.addTab();
  tab.linkedBrowser.addEventListener("load", function(event) {
    tab.linkedBrowser.removeEventListener("load", arguments.callee, true);

    
    
    FullZoom._applyPrefToSetting = function() {
      ok(true, "applyPrefToSetting was called");
      endTest();
    }
    gBrowser.selectedTab = tab;

  }, true); 
  tab.linkedBrowser.loadURI(uri);

  
  
  function endTest() {
    FullZoom._applyPrefToSetting = oldAPTS;
    FullZoom.onLocationChange = oldOLC;
    gBrowser.removeTab(tab);

    oldAPTS = null;
    oldOLC = null;
    tab = null;

    if (gPrefService.prefHasUserValue("browser.zoom.updateBackgroundTabs"))
      gPrefService.clearUserPref("browser.zoom.updateBackgroundTabs");

    if (gPrefService.prefHasUserValue("browser.zoom.siteSpecific"))
      gPrefService.clearUserPref("browser.zoom.siteSpecific");

    finish();
  }

}

