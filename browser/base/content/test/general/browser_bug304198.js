



add_task(function* () {
  let charsToDelete, deletedURLTab, fullURLTab, partialURLTab, testPartialURL, testURL;

  charsToDelete = 5;
  deletedURLTab = gBrowser.addTab();
  fullURLTab = gBrowser.addTab();
  partialURLTab = gBrowser.addTab();
  testURL = "http://example.org/browser/browser/base/content/test/general/dummy_page.html";

  let loaded1 = BrowserTestUtils.browserLoaded(deletedURLTab.linkedBrowser, testURL);
  let loaded2 = BrowserTestUtils.browserLoaded(fullURLTab.linkedBrowser, testURL);
  let loaded3 = BrowserTestUtils.browserLoaded(partialURLTab.linkedBrowser, testURL);
  deletedURLTab.linkedBrowser.loadURI(testURL);
  fullURLTab.linkedBrowser.loadURI(testURL);
  partialURLTab.linkedBrowser.loadURI(testURL);
  yield Promise.all([loaded1, loaded2, loaded3]);

  testURL = gURLBar.trimValue(testURL);
  testPartialURL = testURL.substr(0, (testURL.length - charsToDelete));

  function cleanUp() {
    gBrowser.removeTab(fullURLTab);
    gBrowser.removeTab(partialURLTab);
    gBrowser.removeTab(deletedURLTab);
  }

  function* cycleTabs() {
    yield BrowserTestUtils.switchTab(gBrowser, fullURLTab);
    is(gURLBar.textValue, testURL, 'gURLBar.textValue should be testURL after switching back to fullURLTab');

    yield BrowserTestUtils.switchTab(gBrowser, partialURLTab);
    is(gURLBar.textValue, testPartialURL, 'gURLBar.textValue should be testPartialURL after switching back to partialURLTab');
    yield BrowserTestUtils.switchTab(gBrowser, deletedURLTab);
    is(gURLBar.textValue, '', 'gURLBar.textValue should be "" after switching back to deletedURLTab');

    yield BrowserTestUtils.switchTab(gBrowser, fullURLTab);
    is(gURLBar.textValue, testURL, 'gURLBar.textValue should be testURL after switching back to fullURLTab');
  }

  function urlbarBackspace() {
    return new Promise((resolve, reject) => {
      gBrowser.selectedBrowser.focus();
      gURLBar.addEventListener("input", function () {
        gURLBar.removeEventListener("input", arguments.callee, false);
        resolve();
      }, false);
      gURLBar.focus();
      EventUtils.synthesizeKey("VK_BACK_SPACE", {});
    });
  }

  function* prepareDeletedURLTab() {
    yield BrowserTestUtils.switchTab(gBrowser, deletedURLTab);
    is(gURLBar.textValue, testURL, 'gURLBar.textValue should be testURL after initial switch to deletedURLTab');

    
    gPrefService.setBoolPref("browser.urlbar.clickSelectsAll", true);

    yield urlbarBackspace();
    is(gURLBar.textValue, "", 'gURLBar.textValue should be "" (just set)');
    if (gPrefService.prefHasUserValue("browser.urlbar.clickSelectsAll")) {
      gPrefService.clearUserPref("browser.urlbar.clickSelectsAll");
    }
  }

  function* prepareFullURLTab() {
    yield BrowserTestUtils.switchTab(gBrowser, fullURLTab);
    is(gURLBar.textValue, testURL, 'gURLBar.textValue should be testURL after initial switch to fullURLTab');
  }

  function* preparePartialURLTab() {
    yield BrowserTestUtils.switchTab(gBrowser, partialURLTab);
    is(gURLBar.textValue, testURL, 'gURLBar.textValue should be testURL after initial switch to partialURLTab');

    
    gPrefService.setBoolPref("browser.urlbar.clickSelectsAll", false);

    let deleted = 0;
    while (deleted < charsToDelete) {
      yield urlbarBackspace(arguments.callee);
      deleted++;
    }

    is(gURLBar.textValue, testPartialURL, "gURLBar.textValue should be testPartialURL (just set)");
    if (gPrefService.prefHasUserValue("browser.urlbar.clickSelectsAll")) {
      gPrefService.clearUserPref("browser.urlbar.clickSelectsAll");
    }
  }

  

  
  yield* prepareFullURLTab();
  yield* preparePartialURLTab();
  yield* prepareDeletedURLTab();

  
  yield* cycleTabs();
  cleanUp();
});


