



add_task(function* test_switchtab_override_keynav() {
  let testURL = "http://example.org/browser/browser/base/content/test/general/dummy_page.html";

  info("Opening first tab");
  let tab = gBrowser.addTab(testURL);
  let tabLoadDeferred = Promise.defer();
  whenTabLoaded(tab, tabLoadDeferred.resolve);
  yield tabLoadDeferred.promise;

  info("Opening and selecting second tab");
  let secondTab = gBrowser.selectedTab = gBrowser.addTab();
  registerCleanupFunction(() => {
    try {
      gBrowser.removeTab(tab);
      gBrowser.removeTab(secondTab);
    } catch(ex) {  }
    return PlacesTestUtils.clearHistory();
  });

  gURLBar.focus();
  gURLBar.value = "dummy_pag";
  EventUtils.synthesizeKey("e" , {});
  yield promiseSearchComplete();

  info("Select second autocomplete popup entry");
  EventUtils.synthesizeKey("VK_DOWN" , {});
  ok(/moz-action:switchtab/.test(gURLBar.value), "switch to tab entry found");

  info("Shift+left on switch-to-tab entry");

  EventUtils.synthesizeKey("VK_SHIFT" , { type: "keydown" });
  EventUtils.synthesizeKey("VK_LEFT", { shiftKey: true });
  EventUtils.synthesizeKey("VK_SHIFT" , { type: "keyup" });

  ok(!/moz-action:switchtab/.test(gURLBar.inputField.value), "switch to tab should be hidden");
});
