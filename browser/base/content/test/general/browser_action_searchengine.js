


let gOriginalEngine;

function* promise_first_result(inputText) {
  gURLBar.focus();
  gURLBar.value = inputText.slice(0, -1);
  EventUtils.synthesizeKey(inputText.slice(-1) , {});
  yield promiseSearchComplete();
  
  
  yield promisePopupShown(gURLBar.popup);

  let firstResult = gURLBar.popup.richlistbox.firstChild;
  return firstResult;
}

add_task(function* () {
  
  if (!Services.prefs.getBoolPref("browser.urlbar.unifiedcomplete"))
    return;

  Services.search.addEngineWithDetails("MozSearch", "", "", "", "GET",
                                       "http://example.com/?q={searchTerms}");
  let engine = Services.search.getEngineByName("MozSearch");
  gOriginalEngine = Services.search.currentEngine;
  Services.search.currentEngine = engine;

  let tab = gBrowser.selectedTab = gBrowser.addTab();

  registerCleanupFunction(() => {
    Services.search.currentEngine = gOriginalEngine;
    let engine = Services.search.getEngineByName("MozSearch");
    Services.search.removeEngine(engine);

    try {
      gBrowser.removeTab(tab);
    } catch(ex) {  }

    return promiseClearHistory();
  });

  let result = yield promise_first_result("open a search");
  isnot(result, null, "Should have a result");
  is(result.getAttribute("url"),
     `moz-action:searchengine,{"engineName":"MozSearch","input":"open a search","searchQuery":"open a search"}`,
     "Result should be a moz-action: for the correct search engine");
  is(result.hasAttribute("image"), false, "Result shouldn't have an image attribute");

  let tabPromise = promiseTabLoaded(gBrowser.selectedTab);
  EventUtils.synthesizeMouseAtCenter(result, {});
  yield tabPromise;

  is(gBrowser.selectedBrowser.currentURI.spec, "http://example.com/?q=open+a+search", "Correct URL should be loaded");
});
