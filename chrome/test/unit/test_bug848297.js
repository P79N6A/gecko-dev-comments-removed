




var MANIFESTS = [
  do_get_file("data/test_bug848297.manifest")
];


Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

registerManifests(MANIFESTS);

var chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"]
                .getService(Ci.nsIXULChromeRegistry)
                .QueryInterface(Ci.nsIToolkitChromeRegistry);
chromeReg.checkForNewChrome();

var prefService = Cc["@mozilla.org/preferences-service;1"]
                  .getService(Ci.nsIPrefService)
                  .QueryInterface(Ci.nsIPrefBranch);

function enum_to_array(strings) {
  let rv = [];
  while (strings.hasMore()) {
    rv.push(strings.getNext());
  }
  rv.sort();
  return rv;
}

function run_test() {

  
  prefService.setCharPref("general.useragent.locale", "de");
  do_check_eq(chromeReg.getSelectedLocale("basepack"), "en-US");
  do_check_eq(chromeReg.getSelectedLocale("overpack"), "de");
  do_check_matches(enum_to_array(chromeReg.getLocalesForPackage("basepack")),
                   ['en-US', 'fr']);

  
  prefService.setCharPref("chrome.override_package.basepack", "overpack");
  do_check_eq(chromeReg.getSelectedLocale("basepack"), "de");
  do_check_matches(enum_to_array(chromeReg.getLocalesForPackage("basepack")),
                   ['de', 'en-US']);

}
