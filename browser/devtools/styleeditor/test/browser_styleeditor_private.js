





function test() {
  waitForExplicitFinish();

  gPrefService.setBoolPref("browser.privatebrowsing.keep_current_session", true);
  let pb = Cc["@mozilla.org/privatebrowsing;1"].
           getService(Ci.nsIPrivateBrowsingService);
  pb.privateBrowsingEnabled = true;

  function checkCache() {
    checkDiskCacheFor(TEST_HOST);
    pb.privateBrowsingEnabled = false;
    gPrefService.clearUserPref("browser.privatebrowsing.keep_current_session");
    finish();
  }

  gBrowser.selectedTab = gBrowser.addTab();

  gBrowser.selectedBrowser.addEventListener("load", function onLoad() {
    gBrowser.selectedBrowser.removeEventListener("load", onLoad, true);
    cache.evictEntries(Ci.nsICache.STORE_ANYWHERE);
    launchStyleEditorChrome(function(aChrome) {
      if (aChrome.isContentAttached) {
        onEditorAdded(aChrome, aChrome.editors[0]);
      } else {
        aChrome.addChromeListener({
          onEditorAdded: onEditorAdded
        });
      }
    });
  }, true);

  function onEditorAdded(aChrome, aEditor) {
    aChrome.removeChromeListener(this);

    if (aEditor.isLoaded) {
      checkCache();
    } else {
      aEditor.addActionListener({
        onLoad: checkCache
      });
    }
  }

  content.location = 'http://' + TEST_HOST + '/browser/browser/devtools/styleeditor/test/test_private.html';
}
