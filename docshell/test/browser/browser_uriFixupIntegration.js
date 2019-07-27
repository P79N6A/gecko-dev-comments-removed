


"use strict";

const kSearchEngineID = "browser_urifixup_search_engine";
const kSearchEngineURL = "http://example.com/?search={searchTerms}";

add_task(function* setup() {
  
  Services.search.addEngineWithDetails(kSearchEngineID, "", "", "", "get",
                                       kSearchEngineURL);

  let oldDefaultEngine = Services.search.defaultEngine;
  Services.search.defaultEngine = Services.search.getEngineByName(kSearchEngineID);

  
  registerCleanupFunction(() => {
    if (oldDefaultEngine) {
      Services.search.defaultEngine = oldDefaultEngine;
    }

    let engine = Services.search.getEngineByName(kSearchEngineID);
    if (engine) {
      Services.search.removeEngine(engine);
    }
  });
});

add_task(function* test() {
  for (let searchParams of ["foo bar", "brokenprotocol:somethingelse"]) {
    
    gBrowser.selectedTab = gBrowser.addTab("about:blank");
    yield BrowserTestUtils.browserLoaded(gBrowser.selectedBrowser);

    
    gURLBar.value = searchParams;
    gURLBar.focus();
    EventUtils.synthesizeKey("VK_RETURN", {});
    yield BrowserTestUtils.browserLoaded(gBrowser.selectedBrowser);

    
    let escapedParams = encodeURIComponent(searchParams).replace("%20", "+");
    let expectedURL = kSearchEngineURL.replace("{searchTerms}", escapedParams);
    is(gBrowser.selectedBrowser.currentURI.spec, expectedURL,
       "New tab should have loaded with expected url.");

    
    gBrowser.removeCurrentTab();
  }
});
