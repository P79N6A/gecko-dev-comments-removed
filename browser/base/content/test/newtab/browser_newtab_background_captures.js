







const CAPTURE_PREF = "browser.pagethumbnails.capturing_disabled";

function runTests() {
  let imports = {};
  Cu.import("resource://gre/modules/PageThumbs.jsm", imports);
  Cu.import("resource:///modules/BrowserNewTabPreloader.jsm", imports);

  
  let originalDisabledState = Services.prefs.getBoolPref(CAPTURE_PREF);
  Services.prefs.setBoolPref(CAPTURE_PREF, true);

  
  let siteName = "newtab_background_captures";
  let url = "http://example.com/#" + siteName;
  let path = imports.PageThumbsStorage.getFilePathForURL(url);
  let file = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsIFile);
  file.initWithPath(path);
  try {
    file.remove(false);
  }
  catch (err) {}

  
  yield setLinks(siteName);

  
  
  
  
  
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
  isnot(doc.documentElement.getAttribute("allow-background-captures"), "true",
        "Pre-loaded docshell just synchronously swapped, so background " +
        "captures should not be allowed yet");

  
  Services.prefs.setBoolPref(CAPTURE_PREF, false);

  
  
  let allowBackgroundCaptures = false;
  let mutationObserver = new MutationObserver(() => {
    mutationObserver.disconnect();
    allowBackgroundCaptures = true;
    is(doc.documentElement.getAttribute("allow-background-captures"), "true",
       "allow-background-captures should now be true");
    info("Waiting for thumbnail to be created after observing " +
         "allow-background-captures change");
  });
  mutationObserver.observe(doc.documentElement, {
    attributes: true,
    attributeFilter: ["allow-background-captures"],
  });

  
  
  Services.obs.addObserver(function onCreate(subj, topic, data) {
    if (data != url)
      return;
    ok(allowBackgroundCaptures,
       "page-thumbnail:create should be observed after " +
       "allow-background-captures was set");
    Services.obs.removeObserver(onCreate, "page-thumbnail:create");
    
    Services.prefs.setBoolPref(CAPTURE_PREF, originalDisabledState);
    file.remove(false);
    TestRunner.next();
  }, "page-thumbnail:create", false);

  info("Waiting for allow-background-captures change");
  yield true;
}

function wait(ms) {
  setTimeout(TestRunner.next, ms);
}
