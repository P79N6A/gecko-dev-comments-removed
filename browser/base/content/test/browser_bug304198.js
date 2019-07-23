




































function test() {
  waitForExplicitFinish();

  if (Cc["@mozilla.org/focus-manager;1"].getService(Ci.nsIFocusManager).activeWindow !=
      window) {
    window.addEventListener("focus", function () {
      window.removeEventListener("focus", arguments.callee, false);
      test();
    }, false);
    window.focus();
    return;
  }

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

  function urlbarBackspace(cb) {
    gBrowser.selectedBrowser.focus();
    gURLBar.addEventListener("focus", function () {
      gURLBar.removeEventListener("focus", arguments.callee, false);
      gURLBar.addEventListener("input", function () {
        gURLBar.removeEventListener("input", arguments.callee, false);
        cb();
      }, false);
      executeSoon(function () {
        EventUtils.synthesizeKey("VK_BACK_SPACE", {});
      });
    }, false);
    gURLBar.focus();
  }

  function prepareDeletedURLTab(cb) {
    gBrowser.selectedTab = deletedURLTab;
    is(gURLBar.value, testURL, 'gURLBar.value should be testURL after initial switch to deletedURLTab');

    
    gPrefService.setBoolPref("browser.urlbar.clickSelectsAll", true);

    urlbarBackspace(function () {
      is(gURLBar.value, "", 'gURLBar.value should be "" (just set)');
      gPrefService.clearUserPref("browser.urlbar.clickSelectsAll");
      cb();
    });
  }

  function prepareFullURLTab(cb) {
    gBrowser.selectedTab = fullURLTab;
    is(gURLBar.value, testURL, 'gURLBar.value should be testURL after initial switch to fullURLTab');
    cb();
  }

  function preparePartialURLTab(cb) {
    gBrowser.selectedTab = partialURLTab;
    is(gURLBar.value, testURL, 'gURLBar.value should be testURL after initial switch to partialURLTab');

    
    gPrefService.setBoolPref("browser.urlbar.clickSelectsAll", false);

    var deleted = 0;
    urlbarBackspace(function () {
      deleted++;
      if (deleted < charsToDelete) {
        urlbarBackspace(arguments.callee);
      } else {
        is(gURLBar.value, testPartialURL, "gURLBar.value should be testPartialURL (just set)");
        gPrefService.clearUserPref("browser.urlbar.clickSelectsAll");
        cb();
      }
    });
  }

  function runTests() {
    
    prepareFullURLTab(function () {
      preparePartialURLTab(function () {
        prepareDeletedURLTab(function () {
          
          cycleTabs();
          cleanUp();
          finish();
        });
      });
    });
  }

  load(deletedURLTab, testURL, function() {
    load(fullURLTab, testURL, function() {
      load(partialURLTab, testURL, runTests);
    });
  });
}

