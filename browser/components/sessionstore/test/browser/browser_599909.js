




































const TAB_STATE_NEEDS_RESTORE = 1;
const TAB_STATE_RESTORING = 2;

let stateBackup = ss.getBrowserState();

function cleanup() {
  
  try {
    Services.prefs.clearUserPref("browser.sessionstore.max_concurrent_tabs");
  } catch (e) {}
  ss.setBrowserState(stateBackup);
  executeSoon(finish);
}

function test() {
  
  waitForExplicitFinish();

  
  
  Services.prefs.setIntPref("browser.sessionstore.max_concurrent_tabs", 0);

  let state = { windows: [{ tabs: [
    { entries: [{ url: "http://example.org/#1" }] },
    { entries: [{ url: "http://example.org/#2" }] },
    { entries: [{ url: "http://example.org/#3" }] },
    { entries: [{ url: "http://example.org/#4" }] }
  ], selected: 1 }] };

  let tabsForEnsure = {};
  state.windows[0].tabs.forEach(function(tab) {
    tabsForEnsure[tab.entries[0].url] = 1;
  });

  let tabsRestoring = 0;
  let tabsRestored = 0;

  function handleEvent(aEvent) {
    if (aEvent.type == "SSTabRestoring")
      tabsRestoring++;
    else
      tabsRestored++;

    if (tabsRestoring < state.windows[0].tabs.length ||
        tabsRestored < 1)
      return;

    gBrowser.tabContainer.removeEventListener("SSTabRestoring", handleEvent, true);
    gBrowser.tabContainer.removeEventListener("SSTabRestored", handleEvent, true);
    executeSoon(function() {
      checkAutocompleteResults(tabsForEnsure, cleanup);
    });
  }

  
  
  
  gBrowser.tabContainer.addEventListener("SSTabRestoring", handleEvent, true);
  gBrowser.tabContainer.addEventListener("SSTabRestored", handleEvent, true);
  ss.setBrowserState(JSON.stringify(state));
}



var gController = Cc["@mozilla.org/autocomplete/controller;1"].
                  getService(Ci.nsIAutoCompleteController);

function checkAutocompleteResults(aExpected, aCallback) {
  gController.input = {
    timeout: 10,
    textValue: "",
    searches: ["history"],
    searchParam: "enable-actions",
    popupOpen: false,
    minResultsForPopup: 0,
    invalidate: function() {},
    disableAutoComplete: false,
    completeDefaultIndex: false,
    get popup() { return this; },
    onSearchBegin: function() {},
    onSearchComplete:  function ()
    {
      info("Found " + gController.matchCount + " matches.");
      
      for (let i = 0; i < gController.matchCount; i++) {
        let uri = gController.getValueAt(i).replace(/^moz-action:[^,]+,/i, "");

        info("Search for '" + uri + "' in open tabs.");
        ok(uri in aExpected, "Registered open page found in autocomplete.");
        
        delete aExpected[uri];
      }

      
      for (let entry in aExpected) {
        ok(false, "'" + entry + "' not found in autocomplete.");
      }

      executeSoon(aCallback);
    },
    setSelectedIndex: function() {},
    get searchCount() { return this.searches.length; },
    getSearchAt: function(aIndex) this.searches[aIndex],
    QueryInterface: XPCOMUtils.generateQI([
      Ci.nsIAutoCompleteInput,
      Ci.nsIAutoCompletePopup,
    ])
  };

  info("Searching open pages.");
  gController.startSearch(Services.prefs.getCharPref("browser.urlbar.restrict.openpage"));
}
