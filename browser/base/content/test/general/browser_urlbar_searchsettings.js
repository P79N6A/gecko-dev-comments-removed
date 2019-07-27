


"use strict";

add_task(function*() {
  yield BrowserTestUtils.withNewTab({ gBrowser, url: "about:blank" }, function* () {
    let popupopened = BrowserTestUtils.waitForEvent(gURLBar.popup, "popupshown");

    gURLBar.focus();
    EventUtils.synthesizeKey("a", {});
    yield popupopened;

    
    let loaded = BrowserTestUtils.browserLoaded(gBrowser.selectedBrowser);
    let popupclosed = BrowserTestUtils.waitForEvent(gURLBar.popup, "popuphidden");
    EventUtils.synthesizeMouseAtCenter(document.getElementById("urlbar-search-settings"), {});
    yield loaded;
    yield popupclosed;

    is(gBrowser.selectedBrowser.currentURI.spec, "about:preferences#search",
       "Should have loaded the right page");
  });
});
