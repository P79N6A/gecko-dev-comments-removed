








const TEST_PREFS = [
  ["reader.parse-on-load.enabled", true],
  ["browser.reader.detectedFirstArticle", false],
];

const TEST_PATH = "http://example.com/browser/browser/base/content/test/general/";

let readerButton = document.getElementById("reader-mode-button");

add_task(function* test_reader_button() {
  registerCleanupFunction(function() {
    
    TEST_PREFS.forEach(([name, value]) => {
      Services.prefs.clearUserPref(name);
    });
    while (gBrowser.tabs.length > 1) {
      gBrowser.removeCurrentTab();
    }
  });

  
  TEST_PREFS.forEach(([name, value]) => {
    Services.prefs.setBoolPref(name, value);
  });

  let tab = gBrowser.selectedTab = gBrowser.addTab();
  is_element_hidden(readerButton, "Reader mode button is not present on a new tab");
  
  let url = TEST_PATH + "readerModeArticleHiddenNodes.html";
  yield promiseTabLoadEvent(tab, url);
  yield ContentTask.spawn(tab.linkedBrowser, "", function() {
    return ContentTaskUtils.waitForEvent(content, "MozAfterPaint");
  });

  is_element_hidden(readerButton, "Reader mode button is still not present on tab with unreadable content.");
});
