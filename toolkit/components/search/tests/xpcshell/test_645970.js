


































let Cc = Components.classes;
let Ci = Components.interfaces;

var gPrefService = Cc["@mozilla.org/preferences-service;1"]
                    .getService(Ci.nsIPrefService)
                    .QueryInterface(Ci.nsIPrefBranch);



function run_test() {
  createAppInfo("xpcshell@tests.mozilla.org", "XPCShell", "1", "2");

  do_load_manifest("data/chrome.manifest");

  let url  = "chrome://testsearchplugin/locale/searchplugins/";
  gPrefService.setCharPref("browser.search.jarURIs", url);

  gPrefService.setBoolPref("browser.search.loadFromJars", true);

  
  
  let searchService = Cc["@mozilla.org/browser/search-service;1"]
                       .getService(Ci.nsIBrowserSearchService);
  let engine = searchService.getEngineByName("bug645970");
  do_check_neq(engine, null);
}

