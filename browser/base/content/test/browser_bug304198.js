




































function test() {
  waitForExplicitFinish();

  let charsToDelete, deletedURLTab, fullURLTab, partialURLTab, testPartialURL, testURL;

  charsToDelete = 5;
  deletedURLTab = gBrowser.addTab();
  fullURLTab = gBrowser.addTab();
  partialURLTab = gBrowser.addTab();
  testURL = "http://example.org/browser/browser/base/content/test/dummy_page.html";

  testPartialURL = testURL.substr(0, (testURL.length - charsToDelete));

  function cleanUp() {

    gBrowser.removeTab(fullURLTab);
    gBrowser.removeTab(partialURLTab);
    gBrowser.removeTab(deletedURLTab);
  }

  function cycleTabs() {
    gBrowser.selectedTab = fullURLTab;
    is(gURLBar.value, testURL, 'gURLBar.value should be testURL after switching back to fullURLTab');

    gBrowser.selectedTab = partialURLTab;
    is(gURLBar.value, testPartialURL, 'gURLBar.value should be testPartialURL after switching back to partialURLTab');

    gBrowser.selectedTab = deletedURLTab;
    is(gURLBar.value, '', 'gURLBar.value should be "" after switching back to deletedURLTab');

    gBrowser.selectedTab = fullURLTab;
    is(gURLBar.value, testURL, 'gURLBar.value should be testURL after switching back to fullURLTab');
  }

  
  function load(tab, url, cb) {
    tab.linkedBrowser.addEventListener("load", function (event) {
      event.currentTarget.removeEventListener("load", arguments.callee, true);
      cb();
    }, true);
    tab.linkedBrowser.loadURI(url);
  }

  function prepareDeletedURLTab() {
    gBrowser.selectedTab = deletedURLTab;
    is(gURLBar.value, testURL, 'gURLBar.value should be testURL after initial switch to deletedURLTab');

    
    gPrefService.setBoolPref("browser.urlbar.clickSelectsAll", true);
    gURLBar.focus();

    EventUtils.synthesizeKey("VK_BACK_SPACE", {});
    
    is(gURLBar.value, '', 'gURLBar.value should be "" (just set)');
    gPrefService.clearUserPref("browser.urlbar.clickSelectsAll");
  }

  function prepareFullURLTab() {
    gBrowser.selectedTab = fullURLTab;
    is(gURLBar.value, testURL, 'gURLBar.value should be testURL after initial switch to fullURLTab');
  }

  function preparePartialURLTab() {
    gBrowser.selectedTab = partialURLTab;
    is(gURLBar.value, testURL, 'gURLBar.value should be testURL after initial switch to partialURLTab');

    
    gPrefService.setBoolPref("browser.urlbar.clickSelectsAll", false);
    gURLBar.focus();
    
    for(let i = 0; i < charsToDelete; ++i) {
      EventUtils.synthesizeKey("VK_BACK_SPACE", {});
    }

    is(gURLBar.value, testPartialURL, 'gURLBar.value should be testPartialURL (just set)');
    gPrefService.clearUserPref("browser.urlbar.clickSelectsAll");
  }

  function runTests() {
    
    prepareFullURLTab();
    preparePartialURLTab();
    prepareDeletedURLTab();

    
    cycleTabs();
  }

  load(deletedURLTab, testURL, function() {
    load(fullURLTab, testURL, function() {
      load(partialURLTab, testURL, function() {
        runTests();
        cleanUp();
        finish();
      });
    });
  });
}

