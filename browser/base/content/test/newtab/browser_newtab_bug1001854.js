


const PRELOAD_PREF = "browser.newtab.preload";

function runTests() {
  
  Services.prefs.setBoolPref(PRELOAD_PREF, false);
  Services.prefs.setBoolPref(PREF_NEWTAB_ENABLED, false);
  yield addNewTabPageTab();

  let search = getContentDocument().getElementById("newtab-search-form");
  is(search.style.width, "", "search form has no width yet");

  NewTabUtils.allPages.enabled = true;
  isnot(search.style.width, "", "search form has width set");

  
  Services.prefs.clearUserPref(PRELOAD_PREF);
}
