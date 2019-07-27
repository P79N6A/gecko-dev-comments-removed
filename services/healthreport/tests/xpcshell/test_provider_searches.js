


"use strict";

const {utils: Cu} = Components;

Cu.import("resource://gre/modules/Metrics.jsm");
Cu.import("resource://gre/modules/Services.jsm");
let bsp = Cu.import("resource://gre/modules/services/healthreport/providers.jsm");

const DEFAULT_ENGINES = [
  {name: "Amazon.com",    identifier: "amazondotcom"},
  {name: "Bing",          identifier: "bing"},
  {name: "Google",        identifier: "google"},
  {name: "Yahoo",         identifier: "yahoo"},
  {name: "Foobar Search", identifier: "foobar"},
];

function MockSearchCountMeasurement() {
  bsp.SearchCountMeasurement3.call(this);
}
MockSearchCountMeasurement.prototype = {
  __proto__: bsp.SearchCountMeasurement3.prototype,
};

function MockSearchesProvider() {
  SearchesProvider.call(this);
}
MockSearchesProvider.prototype = {
  __proto__: SearchesProvider.prototype,
  measurementTypes: [MockSearchCountMeasurement],
};

function run_test() {
  
  
  Services.prefs.setBoolPref("browser.search.isUS", true);
  Services.prefs.setCharPref("browser.search.countryCode", "US");

  run_next_test();
}

add_test(function test_constructor() {
  let provider = new SearchesProvider();

  run_next_test();
});

add_task(function* test_record() {
  let storage = yield Metrics.Storage("record");
  let provider = new MockSearchesProvider();

  yield provider.init(storage);

  let now = new Date();

  
  
  for (let engine of DEFAULT_ENGINES.concat([{name: "Not Default", identifier: "notdef"}])) {
    if (engine.identifier == "yahoo") {
      continue;
    }
    yield provider.recordSearch(engine, "abouthome");
    yield provider.recordSearch(engine, "contextmenu");
    yield provider.recordSearch(engine, "newtab");
    yield provider.recordSearch(engine, "searchbar");
    yield provider.recordSearch(engine, "urlbar");
  }

  
  let errored = false;
  try {
    yield provider.recordSearch(DEFAULT_ENGINES[0], "bad source");
  } catch (ex) {
    errored = true;
  } finally {
    do_check_true(errored);
  }

  let m = provider.getMeasurement("counts", 3);
  let data = yield m.getValues();
  do_check_eq(data.days.size, 1);
  do_check_true(data.days.hasDay(now));

  let day = data.days.getDay(now);
  for (let engine of DEFAULT_ENGINES) {
    let identifier = engine.identifier;
    let expected = identifier != "yahoo";

    for (let source of ["abouthome", "contextmenu", "searchbar", "urlbar"]) {
      let field = identifier + "." + source;
      if (expected) {
        do_check_true(day.has(field));
        do_check_eq(day.get(field), 1);
      } else {
        do_check_false(day.has(field));
      }
    }
  }

  
  
  let identifier = "notdef";
  for (let source of ["abouthome", "contextmenu", "searchbar", "urlbar"]) {
    let field = identifier + "." + source;
    do_check_true(day.has(field));
  }

  yield storage.close();
});

add_task(function* test_includes_other_fields() {
  let storage = yield Metrics.Storage("includes_other_fields");
  let provider = new MockSearchesProvider();

  yield provider.init(storage);
  let m = provider.getMeasurement("counts", 3);

  
  let id = yield m.storage.registerField(m.id, "test.searchbar",
                                         Metrics.Storage.FIELD_DAILY_COUNTER);

  let testField = "test.searchbar";
  let now = new Date();
  yield m.storage.incrementDailyCounterFromFieldID(id, now);

  
  do_check_false(testField in m.fields);

  
  do_check_true(m.shouldIncludeField(testField));

  
  let data = yield provider.storage.getMeasurementValues(m.id);
  let serializer = m.serializer(m.SERIALIZE_JSON);
  let formatted = serializer.daily(data.days.getDay(now));
  do_check_true(testField in formatted);
  do_check_eq(formatted[testField], 1);

  yield storage.close();
});

add_task(function* test_default_search_engine() {
  let storage = yield Metrics.Storage("default_search_engine");
  let provider = new SearchesProvider();
  yield provider.init(storage);

  let m = provider.getMeasurement("engines", 1);

  
  Services.prefs.setBoolPref("toolkit.telemetry.enabled", false);

  let now = new Date();
  yield provider.collectDailyData();

  let data = yield m.getValues();
  Assert.equal(data.days.hasDay(now), false);

  
  Services.prefs.setBoolPref("toolkit.telemetry.enabled", true);

  yield provider.collectDailyData();
  data = yield m.getValues();
  Assert.ok(data.days.hasDay(now));

  let day = data.days.getDay(now);
  Assert.equal(day.size, 1);
  Assert.ok(day.has("default"));

  
  Assert.equal(day.get("default"), "NONE");

  Services.search.addEngineWithDetails("testdefault",
                                       "http://localhost/icon.png",
                                       null,
                                       "test description",
                                       "GET",
                                       "http://localhost/search/%s");
  let engine1 = Services.search.getEngineByName("testdefault");
  Assert.ok(engine1);
  Services.search.defaultEngine = engine1;

  yield provider.collectDailyData();
  data = yield m.getValues();
  Assert.equal(data.days.getDay(now).get("default"), "other-testdefault");

  yield storage.close();
});
