







const CAPTURE_PREF = "browser.pagethumbnails.capturing_disabled";

function runTests() {
  let imports = {};
  Cu.import("resource://gre/modules/PageThumbs.jsm", imports);
  Cu.import("resource:///modules/BrowserNewTabPreloader.jsm", imports);

  
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

  
  
  
  
  
  let tab = gWindow.gBrowser.addTab("about:blank");
  yield addNewTabPageTab();
  let swapWaitCount = 0;
  let swapped = imports.BrowserNewTabPreloader.newTab(tab);
  while (!swapped) {
    if (++swapWaitCount == 10) {
      ok(false, "Timed out waiting for newtab docshell swap.");
      return;
    }
    
    yield wait(2000);
    info("Checking newtab swap " + swapWaitCount);
    swapped = imports.BrowserNewTabPreloader.newTab(tab);
  }

  
  let doc = tab.linkedBrowser.contentDocument;

  
  Services.prefs.setBoolPref(CAPTURE_PREF, false);

  
  Services.obs.addObserver(function onCreate(subj, topic, data) {
    if (data != url)
      return;
    Services.obs.removeObserver(onCreate, "page-thumbnail:create");
    
    Services.prefs.setBoolPref(CAPTURE_PREF, originalDisabledState);
    file.remove(false);
    TestRunner.next();
  }, "page-thumbnail:create", false);

  info("Waiting for thumbnail capture");
  yield true;
}

function wait(ms) {
  setTimeout(TestRunner.next, ms);
}
