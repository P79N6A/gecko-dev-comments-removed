





const PREF_GETADDONS_GETSEARCHRESULTS = "extensions.getAddons.search.url";
const NO_MATCH_URL = TESTROOT + "browser_searching_empty.xml";

var gManagerWindow;
var gCategoryUtilities;
var gProvider;

function test() {
  
  Services.prefs.setIntPref(PREF_SEARCH_MAXRESULTS, 15);

  waitForExplicitFinish();

  gProvider = new MockProvider();

  gProvider.createAddons([{
    id: "inlinesettings2@tests.mozilla.org",
    name: "Inline Settings (Regular)",
    version: "1",
    optionsURL: CHROMEROOT + "options.xul",
    optionsType: AddonManager.OPTIONS_TYPE_INLINE
  }]);

  open_manager("addons://list/extension", function(aWindow) {
    gManagerWindow = aWindow;
    gCategoryUtilities = new CategoryUtilities(gManagerWindow);
    run_next_test();
  });
}

function end_test() {
  close_manager(gManagerWindow, finish);
}







function check_is_searching(aExpectedSearching) {
  var loading = gManagerWindow.document.getElementById("search-loading");
  is(!is_hidden(loading), aExpectedSearching,
     "Search throbber should be showing iff currently searching");
}















function search(aQuery, aFinishImmediately, aCallback, aCategoryType) {
  
  Services.prefs.setCharPref(PREF_GETADDONS_GETSEARCHRESULTS, NO_MATCH_URL);

  aCategoryType = aCategoryType ? aCategoryType : "search";

  var searchBox = gManagerWindow.document.getElementById("header-search");
  searchBox.value = aQuery;

  EventUtils.synthesizeMouseAtCenter(searchBox, { }, gManagerWindow);
  EventUtils.synthesizeKey("VK_RETURN", { }, gManagerWindow);

  var finishImmediately = true;
  wait_for_view_load(gManagerWindow, function() {
    is(gCategoryUtilities.selectedCategory, aCategoryType, "Expected category view should be selected");
    is(gCategoryUtilities.isTypeVisible("search"), aCategoryType == "search",
       "Search category should only be visible if it is the current view");
    is(finishImmediately, aFinishImmediately, "Search should finish immediately only if expected");

    aCallback();
  });

  finishImmediately = false
  if (!aFinishImmediately)
    check_is_searching(true);
}








function get_addon_item(aName) {
  var id = aName + "@tests.mozilla.org";
  var list = gManagerWindow.document.getElementById("search-list");
  var rows = list.getElementsByTagName("richlistitem");
  for (var i = 0; i < rows.length; i++) {
    var row = rows[i];
    if (row.mAddon && row.mAddon.id == id)
      return row;
  }

  return null;
}

add_test(function() {
  search("settings", false, function() {
    var localFilter = gManagerWindow.document.getElementById("search-filter-local");
    EventUtils.synthesizeMouseAtCenter(localFilter, { }, gManagerWindow);

    var item = get_addon_item("inlinesettings2");
    
    item.clientTop;
    var button = gManagerWindow.document.getAnonymousElementByAttribute(item, "anonid", "preferences-btn");
    is_element_visible(button, "Preferences button should be visible");

    EventUtils.synthesizeMouseAtCenter(button, { clickCount: 1 }, gManagerWindow);
    wait_for_view_load(gManagerWindow, function() {
      is(gManagerWindow.gViewController.currentViewObj, gManagerWindow.gDetailView, "View should have changed to detail");

      var searchCategory = gManagerWindow.document.getElementById("category-search");
      EventUtils.synthesizeMouseAtCenter(searchCategory, { }, gManagerWindow);
      wait_for_view_load(gManagerWindow, function() {
        is(gManagerWindow.gViewController.currentViewObj, gManagerWindow.gSearchView, "View should have changed back to search");

        
        var remoteFilter = gManagerWindow.document.getElementById("search-filter-remote");
        EventUtils.synthesizeMouseAtCenter(remoteFilter, { }, gManagerWindow);
        run_next_test();
      });
    });
  });
});
