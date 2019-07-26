var tab;

function test() {

  
  

  waitForExplicitFinish();

  gPrefService.setBoolPref("browser.zoom.updateBackgroundTabs", true);
  gPrefService.setBoolPref("browser.zoom.siteSpecific", true);

  let uri = "http://example.org/browser/browser/base/content/test/dummy_page.html";

  Task.spawn(function () {
    tab = gBrowser.addTab();
    yield FullZoomHelper.load(tab, uri);

    
    
    yield FullZoomHelper.selectTabAndWaitForLocationChange(tab);
    ok(true, "applyPrefToSetting was called");
  }).then(endTest, FullZoomHelper.failAndContinue(endTest));
}



function endTest() {
  gBrowser.removeTab(tab);

  tab = null;

  if (gPrefService.prefHasUserValue("browser.zoom.updateBackgroundTabs"))
    gPrefService.clearUserPref("browser.zoom.updateBackgroundTabs");

  if (gPrefService.prefHasUserValue("browser.zoom.siteSpecific"))
    gPrefService.clearUserPref("browser.zoom.siteSpecific");

  finish();
}
