


let gOriginalEngine;
let gEngine;
let gUnifiedCompletePref = "browser.urlbar.unifiedcomplete";
let gRestyleSearchesPref = "browser.urlbar.restyleSearches";



















function promiseAddVisits(aPlaceInfo) {
  return new Promise((resolve, reject) => {
    let places = [];
    if (aPlaceInfo instanceof Ci.nsIURI) {
      places.push({ uri: aPlaceInfo });
    }
    else if (Array.isArray(aPlaceInfo)) {
      places = places.concat(aPlaceInfo);
    } else {
      places.push(aPlaceInfo)
    }

    
    let now = Date.now();
    for (let i = 0, len = places.length; i < len; ++i) {
      if (!places[i].title) {
        places[i].title = "test visit for " + places[i].uri.spec;
      }
      places[i].visits = [{
        transitionType: places[i].transition === undefined ? Ci.nsINavHistoryService.TRANSITION_LINK
                                                           : places[i].transition,
        visitDate: places[i].visitDate || (now++) * 1000,
        referrerURI: places[i].referrer
      }];
    }

    PlacesUtils.asyncHistory.updatePlaces(
      places,
      {
        handleError: function AAV_handleError(aResultCode, aPlaceInfo) {
          let ex = new Components.Exception("Unexpected error in adding visits.",
                                            aResultCode);
          reject(ex);
        },
        handleResult: function () {},
        handleCompletion: function UP_handleCompletion() {
          resolve();
        }
      }
    );
  });
}

registerCleanupFunction(() => {
  Services.prefs.clearUserPref(gUnifiedCompletePref);
  Services.prefs.clearUserPref(gRestyleSearchesPref);
  Services.search.currentEngine = gOriginalEngine;
  Services.search.removeEngine(gEngine);
  return PlacesTestUtils.clearHistory();
});

add_task(function*() {
  Services.prefs.setBoolPref(gUnifiedCompletePref, true);
  Services.prefs.setBoolPref(gRestyleSearchesPref, true);
});

add_task(function*() {

  Services.search.addEngineWithDetails("SearchEngine", "", "", "",
                                       "GET", "http://s.example.com/search");
  gEngine = Services.search.getEngineByName("SearchEngine");
  gEngine.addParam("q", "{searchTerms}", null);
  gOriginalEngine = Services.search.currentEngine;
  Services.search.currentEngine = gEngine;

  let uri = NetUtil.newURI("http://s.example.com/search?q=foo&client=1");
  yield promiseAddVisits({ uri: uri, title: "Foo - SearchEngine Search" });

  let tab = gBrowser.selectedTab = gBrowser.addTab("about:mozilla", {animate: false});
  yield promiseTabLoaded(gBrowser.selectedTab);

  
  
  yield promiseAutocompleteResultPopup("foo");
  let result = gURLBar.popup.richlistbox.children[1];

  isnot(result, null, "Expect a search result");
  is(result.getAttribute("type"), "search favicon", "Expect correct `type` attribute");

  is_element_visible(result._title, "Title element should be visible");
  is_element_visible(result._extraBox, "Extra box element should be visible");

  is(result._extraBox.pack, "start", "Extra box element should start after the title");
  let iconElem = result._extraBox.nextSibling;
  is_element_visible(iconElem,
                     "The element containing the magnifying glass icon should be visible");
  ok(iconElem.classList.contains("ac-result-type-keyword"),
     "That icon should have the same class use for `keyword` results");

  is_element_visible(result._url, "URL element should be visible");
  is(result._url.textContent, "Search with SearchEngine");

  gBrowser.removeCurrentTab();
});
