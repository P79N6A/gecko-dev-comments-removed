





































var MANIFESTS = [
  do_get_file("data/test_bug519468.manifest")
];

registerManifests(MANIFESTS);

var chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"]
                .getService(Ci.nsIXULChromeRegistry)
                .QueryInterface(Ci.nsIToolkitChromeRegistry);

var localeService = Cc["@mozilla.org/intl/nslocaleservice;1"]
                    .getService(Ci.nsILocaleService);

var prefService = Cc["@mozilla.org/preferences-service;1"]
                  .getService(Ci.nsIPrefService)
                  .QueryInterface(Ci.nsIPrefBranch);

function test_locale(aTest) {
  prefService.setBoolPref("intl.locale.matchOS", aTest.matchOS);
  prefService.setCharPref("general.useragent.locale", aTest.selected || "en-US");

  var selectedLocale = chromeReg.getSelectedLocale("testmatchos");
  do_check_eq(selectedLocale, aTest.locale);
}

function run_test()
{
  var systemLocale = localeService.getLocaleComponentForUserAgent();

  var tests = [
    {matchOS: false, selected: null, locale: "en-US"},
    {matchOS: true, selected: null, locale: systemLocale},
    {matchOS: true, selected: "fr-FR", locale: systemLocale},
    {matchOS: false, selected: "fr-FR", locale: "fr-FR"},
    {matchOS: true, selected: null, locale: systemLocale}
  ];

  for (var i = 0; i < tests.length; ++ i) {
    var test = tests[i];
    test_locale(test);
  }
}
