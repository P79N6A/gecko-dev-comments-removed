








const TEST_PREFS = [
  ["reader.parse-on-load.enabled", true],
  ["browser.readinglist.enabled", true],
  ["browser.readinglist.introShown", false],
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
  Services.prefs.setBoolPref("browser.reader.detectedFirstArticle", false);

  let tab = gBrowser.selectedTab = gBrowser.addTab();
  is_element_hidden(readerButton, "Reader mode button is not present on a new tab");
  ok(!UITour.isInfoOnTarget(window, "readerMode-urlBar"),
     "Info panel shouldn't appear without the reader mode button");
  ok(!Services.prefs.getBoolPref("browser.reader.detectedFirstArticle"),
     "Shouldn't have detected the first article");

  
  let url = TEST_PATH + "readerModeArticle.html";
  yield promiseTabLoadEvent(tab, url);
  yield promiseWaitForCondition(() => !readerButton.hidden);
  is_element_visible(readerButton, "Reader mode button is present on a reader-able page");
  ok(UITour.isInfoOnTarget(window, "readerMode-urlBar"),
     "Info panel should be anchored at the reader mode button");
  ok(Services.prefs.getBoolPref("browser.reader.detectedFirstArticle"),
     "Should have detected the first article");

  
  readerButton.click();
  yield promiseTabLoadEvent(tab);
  ok(!UITour.isInfoOnTarget(window, "readerMode-urlBar"), "Info panel should have closed");

  let readerUrl = gBrowser.selectedBrowser.currentURI.spec;
  ok(readerUrl.startsWith("about:reader"), "about:reader loaded after clicking reader mode button");
  is_element_visible(readerButton, "Reader mode button is present on about:reader");

  is(gURLBar.value, readerUrl, "gURLBar value is about:reader URL");
  is(gURLBar.textValue, url.substring("http://".length), "gURLBar is displaying original article URL");

  
  
  let listButton;
  yield promiseWaitForCondition(() =>
    listButton = gBrowser.contentDocument.getElementById("list-button"));
  is_element_visible(listButton, "List button is present on a reader-able page");
  yield promiseWaitForCondition(() => listButton.classList.contains("on"));
  ok(listButton.classList.contains("on"),
    "List button should indicate SideBar-ReadingList open.");
  ok(ReadingListUI.isSidebarOpen,
    "The ReadingListUI should indicate SideBar-ReadingList open.");

  
  listButton.click();
  yield promiseWaitForCondition(() => !listButton.classList.contains("on"));
  ok(!listButton.classList.contains("on"),
    "List button should now indicate SideBar-ReadingList closed.");
  ok(!ReadingListUI.isSidebarOpen,
    "The ReadingListUI should now indicate SideBar-ReadingList closed.");

  
  readerButton.click();
  yield promiseTabLoadEvent(tab);
  is(gBrowser.selectedBrowser.currentURI.spec, url,
    "Original page loaded after clicking active reader mode button");

  
  let newTab = gBrowser.selectedTab = gBrowser.addTab();
  yield promiseTabLoadEvent(newTab, "about:robots");
  yield promiseWaitForCondition(() => readerButton.hidden);
  is_element_hidden(readerButton, "Reader mode button is not present on a non-reader-able page");

  
  gBrowser.removeCurrentTab();
  yield promiseWaitForCondition(() => !readerButton.hidden);
  is_element_visible(readerButton, "Reader mode button is present on a reader-able page");
});

add_task(function* test_getOriginalUrl() {
  let { ReaderMode } = Cu.import("resource://gre/modules/ReaderMode.jsm", {});
  let url = "http://foo.com/article.html";

  is(ReaderMode.getOriginalUrl("about:reader?url=" + encodeURIComponent(url)), url, "Found original URL from encoded URL");
  is(ReaderMode.getOriginalUrl("about:reader?foobar"), null, "Did not find original URL from malformed reader URL");
  is(ReaderMode.getOriginalUrl(url), null, "Did not find original URL from non-reader URL");

  let badUrl = "http://foo.com/?;$%^^";
  is(ReaderMode.getOriginalUrl("about:reader?url=" + encodeURIComponent(badUrl)), badUrl, "Found original URL from encoded malformed URL");
  is(ReaderMode.getOriginalUrl("about:reader?url=" + badUrl), badUrl, "Found original URL from non-encoded malformed URL");
});
