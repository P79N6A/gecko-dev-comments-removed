




var MANIFESTS = [
  do_get_file("data/test_bug519468.manifest")
];


Components.utils.import("resource://gre/modules/XPCOMUtils.jsm");

let stubOSLocale = null;

stubID = Components.ID("9d09d686-d913-414c-a1e6-4be8652d7d93");
localeContractID = "@mozilla.org/intl/nslocaleservice;1";

StubLocaleService = {
  classDescription: "Stub version of Locale service for testing",
  classID:          stubID,
  contractID:       localeContractID,
  QueryInterface:   XPCOMUtils.generateQI([Ci.nsILocaleService, Ci.nsISupports, Ci.nsIFactory]),

  createInstance: function (outer, iid) {
    if (outer)
      throw Components.results.NS_ERROR_NO_AGGREGATION;
    return this.QueryInterface(iid);
  },
  lockFactory: function (lock) {
    throw Components.results.NS_ERROR_NOT_IMPLEMENTED;
  },

  getLocaleComponentForUserAgent: function SLS_getLocaleComponentForUserAgent()
  {
    return stubOSLocale;
  }
}

let registrar = Components.manager.nsIComponentRegistrar;

let localeCID = registrar.contractIDToCID(localeContractID)
let originalFactory =
      Components.manager.getClassObject(Components.classes[localeContractID],
                                        Components.interfaces.nsIFactory);

registrar.registerFactory(stubID, "Unit test Locale Service", localeContractID, StubLocaleService);


do_test_pending()
registerManifests(MANIFESTS);

var chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"]
                .getService(Ci.nsIXULChromeRegistry)
                .QueryInterface(Ci.nsIToolkitChromeRegistry);
chromeReg.checkForNewChrome();

var prefService = Cc["@mozilla.org/preferences-service;1"]
                  .getService(Ci.nsIPrefService)
                  .QueryInterface(Ci.nsIPrefBranch);
var os = Cc["@mozilla.org/observer-service;1"]
         .getService(Ci.nsIObserverService);

var testsLocale = [
  
  {matchOS: false, selected: "en-US", osLocale: "xx-AA", locale: "en-US"},
  {matchOS: true, selected: "en-US", osLocale: "xx-AA", locale: "xx-AA"},
  {matchOS: false, selected: "fr-FR", osLocale: "xx-AA", locale: "fr-FR"},
  {matchOS: true, selected: "fr-FR", osLocale: "xx-AA", locale: "xx-AA"},
  {matchOS: false, selected: "de-DE", osLocale: "xx-AA", locale: "de-DE"},
  {matchOS: true, selected: "de-DE", osLocale: "xx-AA", locale: "xx-AA"},
  
  
  {matchOS: false, selected: "en-US", osLocale: "xx-BB", locale: "en-US"},
  {matchOS: true, selected: "en-US", osLocale: "xx-BB", locale: "xx-AA"},
  {matchOS: false, selected: "fr-FR", osLocale: "xx-BB", locale: "fr-FR"},
  {matchOS: true, selected: "fr-FR", osLocale: "xx-BB", locale: "xx-AA"},
  
  {matchOS: false, selected: "en-US", osLocale: "xy-BB", locale: "en-US"},
  {matchOS: true, selected: "en-US", osLocale: "xy-BB", locale: "en-US"},
  {matchOS: false, selected: "fr-FR", osLocale: "xy-BB", locale: "fr-FR"},
  {matchOS: true, selected: "fr-FR", osLocale: "xy-BB", locale: "en-US"},
];

var observedLocale = null;

function test_locale(aTest) {
  observedLocale = null;

  stubOSLocale = aTest.osLocale;
  prefService.setBoolPref("intl.locale.matchOS", aTest.matchOS);
  prefService.setCharPref("general.useragent.locale", aTest.selected);

  chromeReg.reloadChrome();

  do_check_eq(observedLocale, aTest.locale);
}



function checkValidity() {
  observedLocale = chromeReg.getSelectedLocale("testmatchos");
  dump("checkValidity called back with locale = " + observedLocale + "\n");
}

function run_test() {
  os.addObserver(checkValidity, "selected-locale-has-changed", false);

  for (let count = 0 ; count < testsLocale.length ; count++) {
    dump("count = " + count + " " + testsLocale[count].toSource() + "\n");
    test_locale(testsLocale[count]);
  }

  os.removeObserver(checkValidity, "selected-locale-has-changed");
  do_test_finished();
}
