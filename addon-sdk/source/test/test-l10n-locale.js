



const { getPreferedLocales, findClosestLocale } = require("sdk/l10n/locale");
const prefs = require("sdk/preferences/service");
const { Cc, Ci, Cu } = require("chrome");
const { Services } = Cu.import("resource://gre/modules/Services.jsm");
const BundleService = Cc["@mozilla.org/intl/stringbundle;1"].getService(Ci.nsIStringBundleService);

const PREF_MATCH_OS_LOCALE  = "intl.locale.matchOS";
const PREF_SELECTED_LOCALE  = "general.useragent.locale";
const PREF_ACCEPT_LANGUAGES = "intl.accept_languages";

function assertPrefered(test, expected, msg) {
  test.assertEqual(JSON.stringify(getPreferedLocales()), JSON.stringify(expected),
                   msg);
}

exports.testGetPreferedLocales = function(test) {
  prefs.set(PREF_MATCH_OS_LOCALE, false);
  prefs.set(PREF_SELECTED_LOCALE, "");
  prefs.set(PREF_ACCEPT_LANGUAGES, "");
  assertPrefered(test, ["en-us"],
                 "When all preferences are empty, we only have en-us");

  prefs.set(PREF_SELECTED_LOCALE, "fr");
  prefs.set(PREF_ACCEPT_LANGUAGES, "jp");
  assertPrefered(test, ["fr", "jp", "en-us"],
                 "We first have useragent locale, then web one and finally en-US");

  prefs.set(PREF_SELECTED_LOCALE, "en-US");
  prefs.set(PREF_ACCEPT_LANGUAGES, "en-US");
  assertPrefered(test, ["en-us"],
                 "We do not have duplicates");

  prefs.set(PREF_SELECTED_LOCALE, "en-US");
  prefs.set(PREF_ACCEPT_LANGUAGES, "fr");
  assertPrefered(test, ["en-us", "fr"],
                 "en-US can be first if specified by higher priority preference");

  
  prefs.reset(PREF_MATCH_OS_LOCALE);
  prefs.reset(PREF_SELECTED_LOCALE);
  prefs.reset(PREF_ACCEPT_LANGUAGES);
}




exports.testPreferedLocalizedLocale = function(test) {
  prefs.set(PREF_MATCH_OS_LOCALE, false);
  let bundleURL = "chrome://global/locale/intl.properties";
  prefs.setLocalized(PREF_SELECTED_LOCALE, bundleURL);
  let contentLocale = "ja";
  prefs.set(PREF_ACCEPT_LANGUAGES, contentLocale);

  
  let expectedLocale = BundleService.createBundle(bundleURL).
    GetStringFromName(PREF_SELECTED_LOCALE).
    toLowerCase();

  
  let expectedLocaleList = [expectedLocale];

  
  if (expectedLocaleList.indexOf(contentLocale) == -1)
    expectedLocaleList.push(contentLocale);

  
  if (expectedLocaleList.indexOf("en-us") == -1)
    expectedLocaleList.push("en-us");

  assertPrefered(test, expectedLocaleList, "test localized pref value");

  
  prefs.reset(PREF_MATCH_OS_LOCALE);
  prefs.reset(PREF_SELECTED_LOCALE);
  prefs.reset(PREF_ACCEPT_LANGUAGES);
}

exports.testPreferedOsLocale = function(test) {
  prefs.set(PREF_MATCH_OS_LOCALE, true);
  prefs.set(PREF_SELECTED_LOCALE, "");
  prefs.set(PREF_ACCEPT_LANGUAGES, "");

  let expectedLocale = Services.locale.getLocaleComponentForUserAgent().
    toLowerCase();
  let expectedLocaleList = [expectedLocale];

  
  if (expectedLocale != "en-us")
    expectedLocaleList.push("en-us");

  assertPrefered(test, expectedLocaleList, "Ensure that we select OS locale when related preference is set");

  
  prefs.reset(PREF_MATCH_OS_LOCALE);
  prefs.reset(PREF_SELECTED_LOCALE);
  prefs.reset(PREF_ACCEPT_LANGUAGES);
}

exports.testFindClosestLocale = function(test) {
  
  test.assertEqual(findClosestLocale([], []), null,
                   "When everything is empty we get null");

  test.assertEqual(findClosestLocale(["en", "en-US"], ["en"]),
                   "en", "We always accept exact match first 1/5");
  test.assertEqual(findClosestLocale(["en-US", "en"], ["en"]),
                   "en", "We always accept exact match first 2/5");
  test.assertEqual(findClosestLocale(["en", "en-US"], ["en-us"]),
                   "en-US", "We always accept exact match first 3/5");
  test.assertEqual(findClosestLocale(["ja-JP-mac", "ja", "ja-JP"], ["ja-jp"]),
                   "ja-JP", "We always accept exact match first 4/5");
  test.assertEqual(findClosestLocale(["ja-JP-mac", "ja", "ja-JP"], ["ja-jp-mac"]),
                   "ja-JP-mac", "We always accept exact match first 5/5");

  test.assertEqual(findClosestLocale(["en", "en-GB"], ["en-us"]),
                   "en", "We accept more generic locale, when there is no exact match 1/2");
  test.assertEqual(findClosestLocale(["en-ZA", "en"], ["en-gb"]),
                   "en", "We accept more generic locale, when there is no exact match 2/2");

  test.assertEqual(findClosestLocale(["ja-JP"], ["ja"]),
                   "ja-JP", "We accept more specialized locale, when there is no exact match 1/2");
  
  test.assertEqual(findClosestLocale(["ja-JP-mac", "ja"], ["ja-jp"]),
                   "ja-JP-mac", "We accept more specialized locale, when there is no exact match 2/2");

  test.assertEqual(findClosestLocale(["en-US"], ["en-us"]),
                   "en-US", "We keep the original one as result 1/2");
  test.assertEqual(findClosestLocale(["en-us"], ["en-us"]),
                   "en-us", "We keep the original one as result 2/2");

  test.assertEqual(findClosestLocale(["ja-JP-mac"], ["ja-jp-mac"]),
                   "ja-JP-mac", "We accept locale with 3 parts");
  test.assertEqual(findClosestLocale(["ja-JP"], ["ja-jp-mac"]),
                   "ja-JP", "We accept locale with 2 parts from locale with 3 parts");
  test.assertEqual(findClosestLocale(["ja"], ["ja-jp-mac"]),
                   "ja", "We accept locale with 1 part from locale with 3 parts");
}
