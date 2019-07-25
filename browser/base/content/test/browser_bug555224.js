


const TEST_PAGE = "/browser/browser/base/content/test/dummy_page.html";
var gTestTab, gBgTab, gTestZoom;

function afterZoomAndLoad(aCallback, aTab) {
  let didLoad = false;
  let didZoom = false;
  aTab.linkedBrowser.addEventListener("load", function() {
    aTab.linkedBrowser.removeEventListener("load", arguments.callee, true);
    didLoad = true;
    if (didZoom)
      executeSoon(aCallback);
  }, true);
  let oldAPTS = FullZoom._applyPrefToSetting;
  FullZoom._applyPrefToSetting = function(value, browser) {
    if (!value)
      value = undefined;
    oldAPTS.call(FullZoom, value, browser);
    
    if (browser && browser.currentURI.spec.startsWith("http:")) {
      FullZoom._applyPrefToSetting = oldAPTS;
      didZoom = true;
    }
    if (didLoad && didZoom)
      executeSoon(aCallback);
  };
}

function testBackgroundLoad() {
  is(ZoomManager.zoom, gTestZoom, "opening a background tab should not change foreground zoom");

  gBrowser.removeTab(gBgTab);

  FullZoom.reset();
  gBrowser.removeTab(gTestTab);

  finish();
}

function testInitialZoom() {
  is(ZoomManager.zoom, 1, "initial zoom level should be 1");
  FullZoom.enlarge();

  gTestZoom = ZoomManager.zoom;
  isnot(gTestZoom, 1, "zoom level should have changed");

  afterZoomAndLoad(testBackgroundLoad,
                   gBgTab = gBrowser.loadOneTab("http://mochi.test:8888" + TEST_PAGE,
                                                {inBackground: true}));
}

function test() {
  waitForExplicitFinish();

  gTestTab = gBrowser.addTab();
  gBrowser.selectedTab = gTestTab;

  afterZoomAndLoad(testInitialZoom, gTestTab);
  content.location = "http://example.org" + TEST_PAGE;
}
