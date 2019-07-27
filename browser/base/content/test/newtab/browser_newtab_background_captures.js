







const CAPTURE_PREF = "browser.pagethumbnails.capturing_disabled";

function runTests() {
  let imports = {};
  Cu.import("resource://gre/modules/PageThumbs.jsm", imports);

  
  let originalDisabledState = Services.prefs.getBoolPref(CAPTURE_PREF);
  Services.prefs.setBoolPref(CAPTURE_PREF, true);

  
  let url = "http://example.com/";
  let path = imports.PageThumbsStorage.getFilePathForURL(url);
  let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
  file.initWithPath(path);
  try {
    file.remove(false);
  }
  catch (err) {}

  
  yield setLinks("-1");

  
  
  gBrowser._createPreloadBrowser();

  
  yield waitForBrowserLoad(gBrowser._preloadedBrowser);

  
  BrowserOpenTab();
  let tab = gBrowser.selectedTab;
  let doc = tab.linkedBrowser.contentDocument;

  
  Services.prefs.setBoolPref(CAPTURE_PREF, false);

  
  Services.obs.addObserver(function onCreate(subj, topic, data) {
    if (data != url)
      return;
    Services.obs.removeObserver(onCreate, "page-thumbnail:create");
    ok(true, "thumbnail created after preloaded tab was shown");

    
    Services.prefs.setBoolPref(CAPTURE_PREF, originalDisabledState);
    gBrowser.removeTab(tab);
    file.remove(false);
    TestRunner.next();
  }, "page-thumbnail:create", false);

  info("Waiting for thumbnail capture");
  yield true;
}
