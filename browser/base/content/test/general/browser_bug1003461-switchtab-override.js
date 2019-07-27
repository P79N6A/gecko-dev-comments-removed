



add_task(function* test_switchtab_override() {
  let testURL = "http://example.org/browser/browser/base/content/test/general/dummy_page.html";

  info("Opening first tab");
  let tab = gBrowser.addTab(testURL);
  let deferred = Promise.defer();
  whenTabLoaded(tab, deferred.resolve);
  yield deferred.promise;

  info("Opening and selecting second tab");
  let secondTab = gBrowser.selectedTab = gBrowser.addTab();
  registerCleanupFunction(() => {
    try {
      gBrowser.removeTab(tab);
      gBrowser.removeTab(secondTab);
    } catch(ex) {  }
  });

  info("Wait for autocomplete")
  deferred = Promise.defer();
  let onSearchComplete = gURLBar.onSearchComplete;
  registerCleanupFunction(() => {
    gURLBar.onSearchComplete = onSearchComplete;
  });
  gURLBar.onSearchComplete = function () {
    ok(gURLBar.popupOpen, "The autocomplete popup is correctly open");
    onSearchComplete.apply(gURLBar);
    deferred.resolve();
  }
  
  gURLBar.focus();
  gURLBar.value = "dummy_pag";
  EventUtils.synthesizeKey("e" , {});
  yield deferred.promise;

  info("Select first autocomplete popup entry");
  EventUtils.synthesizeKey("VK_DOWN" , {});
  ok(/moz-action:switchtab/.test(gURLBar.value), "switch to tab entry found");

  info("Override switch-to-tab");
  deferred = Promise.defer();
  
  let onTabSelect = event => {
    deferred.reject(new Error("Should have overridden switch to tab"));
  };
  gBrowser.tabContainer.addEventListener("TabSelect", onTabSelect, false);
  registerCleanupFunction(() => {
    gBrowser.tabContainer.removeEventListener("TabSelect", onTabSelect, false);
  });
  
  whenTabLoaded(secondTab, deferred.resolve);

  EventUtils.synthesizeKey("VK_SHIFT" , { type: "keydown" });
  EventUtils.synthesizeKey("VK_RETURN" , { });
  EventUtils.synthesizeKey("VK_SHIFT" , { type: "keyup" });
  yield deferred.promise;

  yield promiseClearHistory();
});
