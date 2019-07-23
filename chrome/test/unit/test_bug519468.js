





































function write_locale(stream, locale, package) {
  var s = "locale " + package + " " + locale + " jar:" + locale + ".jar!";
  s += "/locale/" + locale + "/" + package +"/\n";
  stream.write(s, s.length);
}

var localeService = Cc["@mozilla.org/intl/nslocaleservice;1"]
                    .getService(Ci.nsILocaleService);

var systemLocale = localeService.getLocaleComponentForUserAgent();

var locales;

if (systemLocale == "en-US")
  locales = [ "en-US", "fr-FR", "de-DE" ];
else if (systemLocale == "fr-FR")
  locales = [ "en-US", systemLocale, "de-DE" ];
else
  locales = [ "en-US", systemLocale, "fr-FR" ];

var workingDir = Cc["@mozilla.org/file/directory_service;1"].
                 getService(Ci.nsIProperties).get("TmpD", Ci.nsIFile);
var manifest = Cc["@mozilla.org/file/local;1"].createInstance(Ci.nsILocalFile);
manifest.initWithFile(workingDir);
manifest.append("test_bug519468.manifest");
manifest.createUnique(Ci.nsIFile.NORMAL_FILE_TYPE, 0600);
var stream = Cc["@mozilla.org/network/file-output-stream;1"].
             createInstance(Ci.nsIFileOutputStream);
stream.init(manifest, 0x04 | 0x08 | 0x20, 0600, 0); 
locales.slice(0,2).forEach(function(l) write_locale(stream, l, "testmatchos"));
write_locale(stream, locales[2], "testnomatchos");
stream.close();

var MANIFESTS = [
  manifest
];

registerManifests(MANIFESTS);

var chromeReg = Cc["@mozilla.org/chrome/chrome-registry;1"]
                .getService(Ci.nsIXULChromeRegistry)
                .QueryInterface(Ci.nsIToolkitChromeRegistry);

var prefService = Cc["@mozilla.org/preferences-service;1"]
                  .getService(Ci.nsIPrefService)
                  .QueryInterface(Ci.nsIPrefBranch);

function test_locale(aTest) {
  prefService.setBoolPref("intl.locale.matchOS", aTest.matchOS);
  if (aTest.selected)
    prefService.setCharPref("general.useragent.locale", aTest.selected);
  else
    try {
      prefService.clearUserPref("general.useragent.locale");
    } catch(e) {}

  var selectedLocale = chromeReg.getSelectedLocale(aTest.package);
  do_check_eq(selectedLocale, aTest.locale);
}

function run_test()
{
  var tests = [
    {matchOS: true, selected: null, locale: systemLocale},
    {matchOS: true, selected: locales[0], locale: locales[0]},
    {matchOS: true, selected: locales[1], locale: locales[1]},
    {matchOS: true, selected: locales[2], locale: locales[0]},
    {matchOS: true, selected: null, locale: locales[2], package: "testnomatchos"},
    {matchOS: false, selected: null, locale: locales[0]},
    {matchOS: false, selected: locales[0], locale: locales[0]},
    {matchOS: false, selected: locales[1], locale: locales[1]},
    {matchOS: false, selected: locales[2], locale: locales[0]},
  ];

  for (var i = 0; i < tests.length; ++ i) {
    var test = tests[i];
    if (!test.package)
      test.package = "testmatchos";
    test_locale(test);
  }
  manifest.remove(false);
}
