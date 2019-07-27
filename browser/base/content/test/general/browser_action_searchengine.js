


let gOriginalEngine;

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

  gURLBar.focus();
  gURLBar.value = "open a searc";
  EventUtils.synthesizeKey("h" , {});
  yield promiseSearchComplete();

  EventUtils.synthesizeKey("VK_RETURN" , { });
  yield promiseTabLoaded(gBrowser.selectedTab);

  is(gBrowser.selectedBrowser.currentURI.spec, "http://example.com/?q=open+a+search");
});
