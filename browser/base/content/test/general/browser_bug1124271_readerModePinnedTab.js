





const PREF = "reader.parse-on-load.enabled";

const TEST_PATH = "http://example.com/browser/browser/base/content/test/general/";

let readerButton = document.getElementById("reader-mode-button");

add_task(function* () {
  registerCleanupFunction(function() {
    Services.prefs.clearUserPref(PREF);
    while (gBrowser.tabs.length > 1) {
      gBrowser.removeCurrentTab();
    }
  });

  
  Services.prefs.setBoolPref(PREF, true);

  let tab = gBrowser.selectedTab = gBrowser.addTab();
  gBrowser.pinTab(tab);

  let initialTabsCount = gBrowser.tabs.length;

  
  let url = TEST_PATH + "readerModeArticle.html";
  yield promiseTabLoadEvent(tab, url);
  yield promiseWaitForCondition(() => !readerButton.hidden);

  readerButton.click();
  yield promiseTabLoadEvent(tab);

  
  is(gBrowser.tabs.length, initialTabsCount, "No additional tabs were opened.");

  readerButton.click();
  yield promiseTabLoadEvent(tab);

  
  is(gBrowser.tabs.length, initialTabsCount, "No additional tabs were opened.");

  gBrowser.removeCurrentTab();
});
