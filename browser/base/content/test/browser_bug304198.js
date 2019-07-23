




































function test() {
  waitForExplicitFinish();

  let deletedURLTab, fullURLTab, partialURLTab, testPartialURL, testURL;

  deletedURLTab = gBrowser.addTab();
  fullURLTab = gBrowser.addTab();
  partialURLTab = gBrowser.addTab();
  testPartialURL = "http://example.org/brow";
  testURL = "http://example.org/browser/browser/base/content/test/dummy_page.html";

  function cleanUp() {

    gBrowser.removeTab(fullURLTab);
    gBrowser.removeTab(partialURLTab);
    gBrowser.removeTab(deletedURLTab);
  }

  
  function load(tab, url, cb) {
    tab.linkedBrowser.addEventListener("load", function (event) {
      event.currentTarget.removeEventListener("load", arguments.callee, true);
      cb();
    }, true);
    tab.linkedBrowser.loadURI(url);
  }

  function runTests() {
    gBrowser.selectedTab = fullURLTab;
    is(gURLBar.value, testURL, 'gURLBar.value should be testURL after initial switch to fullURLTab');

    gBrowser.selectedTab = partialURLTab;
    is(gURLBar.value, testURL, 'gURLBar.value should be testURL after initial switch to partialURLTab');

    
    gBrowser.userTypedValue = testPartialURL;
    URLBarSetURI();
    is(gURLBar.value, testPartialURL, 'gURLBar.value should be testPartialURL (just set)');

    gBrowser.selectedTab = deletedURLTab;
    is(gURLBar.value, testURL, 'gURLBar.value should be testURL after initial switch to deletedURLTab');

    
    gBrowser.userTypedValue = '';
    URLBarSetURI();
    is(gURLBar.value, '', 'gURLBar.value should be "" (just set)');

    
    gBrowser.selectedTab = fullURLTab;
    is(gURLBar.value, testURL, 'gURLBar.value should be testURL after switching back to fullURLTab');

    gBrowser.selectedTab = partialURLTab;
    is(gURLBar.value, testPartialURL, 'gURLBar.value should be testPartialURL after switching back to partialURLTab');

    gBrowser.selectedTab = deletedURLTab;
    is(gURLBar.value, '', 'gURLBar.value should be "" after switching back to deletedURLTab');

    gBrowser.selectedTab = fullURLTab;
    is(gURLBar.value, testURL, 'gURLBar.value should be testURL after switching back to fullURLTab');
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

