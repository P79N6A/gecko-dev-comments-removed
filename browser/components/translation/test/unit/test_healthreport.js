


"use strict";

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/Services.jsm", this);
Cu.import("resource://gre/modules/Metrics.jsm", this);
Cu.import("resource:///modules/translation/Translation.jsm", this);
Cu.import("resource://testing-common/services/healthreport/utils.jsm", this);


const ORIGINAL_TELEMETRY_ENABLED = Services.prefs.getBoolPref("toolkit.telemetry.enabled");

function run_test() {
  run_next_test();
}

add_test(function setup() {
  do_get_profile();
  Services.prefs.setBoolPref("toolkit.telemetry.enabled", true);

  run_next_test();
});

do_register_cleanup(function() {
  Services.prefs.setBoolPref("toolkit.telemetry.enabled",
                             ORIGINAL_TELEMETRY_ENABLED);
});

add_task(function test_constructor() {
  let provider = new TranslationProvider();
});


add_task(function* test_init() {
  let storage = yield Metrics.Storage("init");
  let provider = new TranslationProvider();
  yield provider.init(storage);
  yield provider.shutdown();
  yield storage.close();
});


add_task(function* test_translation_opportunity() {
  let storage = yield Metrics.Storage("opportunity");
  let provider = new TranslationProvider();
  yield provider.init(storage);

  
  let now = new Date();
  let m = provider.getMeasurement("translation", 1);
  let values = yield m.getValues();
  Assert.equal(values.days.size, 0);
  Assert.ok(!values.days.hasDay(now));

  
  yield provider.recordTranslationOpportunity("fr", now);

  values = yield m.getValues();
  Assert.equal(values.days.size, 1);
  Assert.ok(values.days.hasDay(now));
  let day = values.days.getDay(now);
  Assert.ok(day.has("translationOpportunityCount"));
  Assert.equal(day.get("translationOpportunityCount"), 1);

  Assert.ok(day.has("translationOpportunityCountsByLanguage"));
  let countsByLanguage = JSON.parse(day.get("translationOpportunityCountsByLanguage"));
  Assert.equal(countsByLanguage["fr"], 1);

  
  yield provider.recordMissedTranslationOpportunity("it", now);

  values = yield m.getValues();
  day = values.days.getDay(now);
  Assert.equal(values.days.size, 1);
  Assert.ok(values.days.hasDay(now));
  Assert.ok(day.has("missedTranslationOpportunityCount"));
  Assert.equal(day.get("missedTranslationOpportunityCount"), 1);

  Assert.ok(day.has("missedTranslationOpportunityCountsByLanguage"));
  let missedCountsByLanguage = JSON.parse(day.get("missedTranslationOpportunityCountsByLanguage"));
  Assert.equal(missedCountsByLanguage["it"], 1);

  
  yield provider.recordTranslationOpportunity("fr", now);
  yield provider.recordTranslationOpportunity("fr", now);
  yield provider.recordTranslationOpportunity("es", now);

  yield provider.recordMissedTranslationOpportunity("it", now);
  yield provider.recordMissedTranslationOpportunity("cs", now);
  yield provider.recordMissedTranslationOpportunity("fi", now);

  values = yield m.getValues();
  day = values.days.getDay(now);
  Assert.ok(day.has("translationOpportunityCount"));
  Assert.equal(day.get("translationOpportunityCount"), 4);
  Assert.ok(day.has("missedTranslationOpportunityCount"));
  Assert.equal(day.get("missedTranslationOpportunityCount"), 4);

  Assert.ok(day.has("translationOpportunityCountsByLanguage"));
  countsByLanguage = JSON.parse(day.get("translationOpportunityCountsByLanguage"));
  Assert.equal(countsByLanguage["fr"], 3);
  Assert.equal(countsByLanguage["es"], 1);

  Assert.ok(day.has("missedTranslationOpportunityCountsByLanguage"));
  missedCountsByLanguage = JSON.parse(day.get("missedTranslationOpportunityCountsByLanguage"));
  Assert.equal(missedCountsByLanguage["it"], 2);
  Assert.equal(missedCountsByLanguage["cs"], 1);
  Assert.equal(missedCountsByLanguage["fi"], 1);

  yield provider.shutdown();
  yield storage.close();
});


add_task(function* test_record_translation() {
  let storage = yield Metrics.Storage("translation");
  let provider = new TranslationProvider();
  yield provider.init(storage);
  let now = new Date();

  
  yield provider.recordTranslation("fr", "es", 1000, now);

  let m = provider.getMeasurement("translation", 1);
  let values = yield m.getValues();
  Assert.equal(values.days.size, 1);
  Assert.ok(values.days.hasDay(now));
  let day = values.days.getDay(now);
  Assert.ok(day.has("pageTranslatedCount"));
  Assert.equal(day.get("pageTranslatedCount"), 1);
  Assert.ok(day.has("charactersTranslatedCount"));
  Assert.equal(day.get("charactersTranslatedCount"), 1000);

  Assert.ok(day.has("pageTranslatedCountsByLanguage"));
  let countsByLanguage = JSON.parse(day.get("pageTranslatedCountsByLanguage"));
  Assert.ok("fr" in countsByLanguage);
  Assert.equal(countsByLanguage["fr"]["total"], 1);
  Assert.equal(countsByLanguage["fr"]["es"], 1);

  
  yield provider.recordTranslation("fr", "es", 1, now);
  yield provider.recordTranslation("fr", "en", 2, now);
  yield provider.recordTranslation("es", "en", 4, now);

  values = yield m.getValues();
  day = values.days.getDay(now);
  Assert.ok(day.has("pageTranslatedCount"));
  Assert.equal(day.get("pageTranslatedCount"), 4);
  Assert.ok(day.has("charactersTranslatedCount"));
  Assert.equal(day.get("charactersTranslatedCount"), 1007);

  Assert.ok(day.has("pageTranslatedCountsByLanguage"));
  countsByLanguage = JSON.parse(day.get("pageTranslatedCountsByLanguage"));
  Assert.ok("fr" in countsByLanguage);
  Assert.equal(countsByLanguage["fr"]["total"], 3);
  Assert.equal(countsByLanguage["fr"]["es"], 2);
  Assert.equal(countsByLanguage["fr"]["en"], 1);
  Assert.ok("es" in countsByLanguage);
  Assert.equal(countsByLanguage["es"]["total"], 1);
  Assert.equal(countsByLanguage["es"]["en"], 1);

  yield provider.shutdown();
  yield storage.close();
});


add_task(function* test_record_translation() {
  let storage = yield Metrics.Storage("translation");
  let provider = new TranslationProvider();
  yield provider.init(storage);
  let now = new Date();

  
  yield provider.recordDetectedLanguageChange(true);

  
  yield provider.recordDetectedLanguageChange(false);
  yield provider.recordDetectedLanguageChange(false);

  
  yield provider.recordTargetLanguageChange();
  yield provider.recordTargetLanguageChange();

  let m = provider.getMeasurement("translation", 1);
  let values = yield m.getValues();
  Assert.equal(values.days.size, 1);
  Assert.ok(values.days.hasDay(now));
  let day = values.days.getDay(now);

  Assert.ok(day.has("detectedLanguageChangedBefore"));
  Assert.equal(day.get("detectedLanguageChangedBefore"), 1);

  Assert.ok(day.has("detectedLanguageChangedAfter"));
  Assert.equal(day.get("detectedLanguageChangedAfter"), 2);
  Assert.ok(day.has("targetLanguageChanged"));
  Assert.equal(day.get("targetLanguageChanged"), 2);

  yield provider.shutdown();
  yield storage.close();
});

function* test_simple_counter(aProviderFuncName, aCounterName) {
  let storage = yield Metrics.Storage("translation");
  let provider = new TranslationProvider();
  yield provider.init(storage);
  let now = new Date();

  yield provider[aProviderFuncName]();
  yield provider[aProviderFuncName]();

  let m = provider.getMeasurement("translation", 1);
  let values = yield m.getValues();
  Assert.equal(values.days.size, 1);
  Assert.ok(values.days.hasDay(now));
  let day = values.days.getDay(now);

  Assert.ok(day.has(aCounterName));
  Assert.equal(day.get(aCounterName), 2);

  yield provider.shutdown();
  yield storage.close();
}

add_task(function* test_denied_translation_offer() {
  yield test_simple_counter("recordDeniedTranslationOffer", "deniedTranslationOffer");
});

add_task(function* test_show_original() {
  yield test_simple_counter("recordShowOriginalContent", "showOriginalContent");  
});

add_task(function* test_show_original() {
  yield test_simple_counter("recordAutoRejectedTranslationOffer",
                            "autoRejectedTranslationOffer");
});

add_task(function* test_collect_daily() {
  let storage = yield Metrics.Storage("translation");
  let provider = new TranslationProvider();
  yield provider.init(storage);
  let now = new Date();

  
  const kPrefDetectLanguage = "browser.translation.detectLanguage";
  const kPrefShowUI = "browser.translation.ui.show";
  Services.prefs.setBoolPref(kPrefDetectLanguage, false);
  Services.prefs.setBoolPref(kPrefShowUI, false);

  
  yield provider.collectDailyData();

  let m = provider.getMeasurement("translation", 1);
  let values = yield m.getValues();
  Assert.equal(values.days.size, 1);
  Assert.ok(values.days.hasDay(now));
  let day = values.days.getDay(now);
  Assert.ok(day.has("detectLanguageEnabled"));
  Assert.ok(day.has("showTranslationUI"));

  
  Services.prefs.setBoolPref(kPrefDetectLanguage, true);
  Services.prefs.setBoolPref(kPrefShowUI, true);

  yield provider.collectDailyData();

  values = yield m.getValues();
  day = values.days.getDay(now);
  Assert.equal(day.get("detectLanguageEnabled"), 1);
  Assert.equal(day.get("showTranslationUI"), 1);

  yield provider.shutdown();
  yield storage.close();
});


add_task(function* test_healthreporter_json() {
  Services.prefs.setBoolPref("toolkit.telemetry.enabled", true);

  let reporter = yield getHealthReporter("healthreporter_json");
  yield reporter.init();
  try {
    let now = new Date();
    let provider = new TranslationProvider();
    yield reporter._providerManager.registerProvider(provider);

    yield provider.recordTranslationOpportunity("fr", now);
    yield provider.recordDetectedLanguageChange(true);
    yield provider.recordTranslation("fr", "en", 1000, now);
    yield provider.recordDetectedLanguageChange(false);

    yield provider.recordTranslationOpportunity("es", now);
    yield provider.recordTranslation("es", "en", 1000, now);

    yield provider.recordDeniedTranslationOffer();

    yield provider.recordAutoRejectedTranslationOffer();

    yield provider.recordShowOriginalContent();

    yield reporter.collectMeasurements();
    let payload = yield reporter.getJSONPayload(true);
    let today = reporter._formatDate(now);

    Assert.ok(today in payload.data.days);
    let day = payload.data.days[today];

    Assert.ok("org.mozilla.translation.translation" in day);

    let translations = day["org.mozilla.translation.translation"];

    Assert.equal(translations["translationOpportunityCount"], 2);
    Assert.equal(translations["pageTranslatedCount"], 2);
    Assert.equal(translations["charactersTranslatedCount"], 2000);

    Assert.ok("translationOpportunityCountsByLanguage" in translations);
    Assert.equal(translations["translationOpportunityCountsByLanguage"]["fr"], 1);
    Assert.equal(translations["translationOpportunityCountsByLanguage"]["es"], 1);

    Assert.ok("pageTranslatedCountsByLanguage" in translations);
    Assert.ok("fr" in translations["pageTranslatedCountsByLanguage"]);
    Assert.equal(translations["pageTranslatedCountsByLanguage"]["fr"]["total"], 1);
    Assert.equal(translations["pageTranslatedCountsByLanguage"]["fr"]["en"], 1);

    Assert.ok("es" in translations["pageTranslatedCountsByLanguage"]);
    Assert.equal(translations["pageTranslatedCountsByLanguage"]["es"]["total"], 1);
    Assert.equal(translations["pageTranslatedCountsByLanguage"]["es"]["en"], 1);

    Assert.ok("detectedLanguageChangedBefore" in translations);
    Assert.equal(translations["detectedLanguageChangedBefore"], 1);
    Assert.ok("detectedLanguageChangedAfter" in translations);
    Assert.equal(translations["detectedLanguageChangedAfter"], 1);
    
    Assert.ok("deniedTranslationOffer" in translations);
    Assert.equal(translations["deniedTranslationOffer"], 1);

    Assert.ok("autoRejectedTranslationOffer" in translations);
    Assert.equal(translations["autoRejectedTranslationOffer"], 1);

    Assert.ok("showOriginalContent" in translations);
    Assert.equal(translations["showOriginalContent"], 1);
  } finally {
    reporter._shutdown();
  }
});


add_task(function* test_healthreporter_json2() {
  Services.prefs.setBoolPref("toolkit.telemetry.enabled", false);

  let reporter = yield getHealthReporter("healthreporter_json");
  yield reporter.init();
  try {
    let now = new Date();
    let provider = new TranslationProvider();
    yield reporter._providerManager.registerProvider(provider);

    yield provider.recordTranslationOpportunity("fr", now);
    yield provider.recordDetectedLanguageChange(true);
    yield provider.recordTranslation("fr", "en", 1000, now);
    yield provider.recordDetectedLanguageChange(false);

    yield provider.recordTranslationOpportunity("es", now);
    yield provider.recordTranslation("es", "en", 1000, now);

    yield provider.recordDeniedTranslationOffer();

    yield provider.recordAutoRejectedTranslationOffer();

    yield provider.recordShowOriginalContent();

    yield reporter.collectMeasurements();
    let payload = yield reporter.getJSONPayload(true);
    let today = reporter._formatDate(now);

    Assert.ok(today in payload.data.days);
    let day = payload.data.days[today];

    Assert.ok("org.mozilla.translation.translation" in day);

    let translations = day["org.mozilla.translation.translation"];

    Assert.ok(!("translationOpportunityCount" in translations));
    Assert.ok(!("pageTranslatedCount" in translations));
    Assert.ok(!("charactersTranslatedCount" in translations));
    Assert.ok(!("translationOpportunityCountsByLanguage" in translations));
    Assert.ok(!("pageTranslatedCountsByLanguage" in translations));
    Assert.ok(!("detectedLanguageChangedBefore" in translations));
    Assert.ok(!("detectedLanguageChangedAfter" in translations));
    Assert.ok(!("deniedTranslationOffer" in translations));
    Assert.ok(!("autoRejectedTranslationOffer" in translations));
    Assert.ok(!("showOriginalContent" in translations));
  } finally {
    reporter._shutdown();
  }
});
