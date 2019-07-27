








const TEST_PREFS = [
  ["reader.parse-on-load.enabled", true],
  ["browser.readinglist.enabled", true],
  ["browser.readinglist.introShown", false],
];

const TEST_PATH = "http://example.com/browser/browser/base/content/test/general/";

let readerButton = document.getElementById("reader-mode-button");

add_task(function* () {
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

  
  let url = TEST_PATH + "readerModeArticle.html";
  yield promiseTabLoadEvent(tab, url);
  yield promiseWaitForCondition(() => !readerButton.hidden);
  is_element_visible(readerButton, "Reader mode button is present on a reader-able page");

  
  readerButton.click();
  yield promiseTabLoadEvent(tab);

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
