



add_task(function*() {
  let bm = yield PlacesUtils.bookmarks.insert({ parentGuid: PlacesUtils.bookmarks.unfiledGuid,
                                                url: "http://example.com/",
                                                title: "test" });

  registerCleanupFunction(function* () {
    yield PlacesUtils.bookmarks.remove(bm);
  });

  
  let ucpref = Services.prefs.getBoolPref("browser.urlbar.unifiedcomplete");
  registerCleanupFunction(() => {
    Services.prefs.setBoolPref("browser.urlbar.unifiedcomplete", ucpref);
  });

  Services.prefs.setBoolPref("browser.urlbar.unifiedcomplete", false);
  yield BrowserTestUtils.withNewTab({ gBrowser, url: "about:blank" }, testDelete);

  Services.prefs.setBoolPref("browser.urlbar.unifiedcomplete", true);
  yield BrowserTestUtils.withNewTab({ gBrowser, url: "about:blank" }, testDelete);
});

function sendHome() {
  
  if (Services.appinfo.OS == "Darwin") {
    EventUtils.synthesizeKey("VK_LEFT", { altKey: true });
  } else {
    EventUtils.synthesizeKey("VK_HOME", {});
  }
}

function sendDelete() {
  EventUtils.synthesizeKey("VK_DELETE", {});
}

function* testDelete() {
  yield promiseAutocompleteResultPopup("exam");

  
  sendHome();
  
  sendDelete();
  Assert.equal(gURLBar.inputField.value, "xam");

  yield promisePopupShown(gURLBar.popup);

  sendDelete();
  Assert.equal(gURLBar.inputField.value, "am");
}
