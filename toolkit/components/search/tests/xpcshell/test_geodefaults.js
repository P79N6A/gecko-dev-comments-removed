


"use strict";

var requests = [];
var gServerCohort = "";

const kUrlPref = "geoSpecificDefaults.url";

const kDayInSeconds = 86400;
const kYearInSeconds = kDayInSeconds * 365;

function run_test() {
  updateAppInfo();
  installTestEngine();

  let srv = new HttpServer();

  srv.registerPathHandler("/lookup_defaults", (metadata, response) => {
    response.setStatusLine("1.1", 200, "OK");
    let data = {interval: kYearInSeconds,
                settings: {searchDefault: "Test search engine"}};
    if (gServerCohort)
      data.cohort = gServerCohort;
    response.write(JSON.stringify(data));
    requests.push(metadata);
  });

  srv.registerPathHandler("/lookup_fail", (metadata, response) => {
    response.setStatusLine("1.1", 404, "Not Found");
    requests.push(metadata);
  });

  srv.registerPathHandler("/lookup_unavailable", (metadata, response) => {
    response.setStatusLine("1.1", 503, "Service Unavailable");
    response.setHeader("Retry-After", kDayInSeconds.toString());
    requests.push(metadata);
  });

  srv.start(-1);
  do_register_cleanup(() => srv.stop(() => {}));

  let url = "http://localhost:" + srv.identity.primaryPort + "/lookup_defaults?";
  Services.prefs.getDefaultBranch(BROWSER_SEARCH_PREF).setCharPref(kUrlPref, url);
  
  Services.prefs.setCharPref(BROWSER_SEARCH_PREF + kUrlPref, "about:blank");
  Services.prefs.setCharPref("browser.search.geoip.url",
                             'data:application/json,{"country_code": "FR"}');

  run_next_test();
}

function checkNoRequest() {
  do_check_eq(requests.length, 0);
}

function checkRequest(cohort = "") {
  do_check_eq(requests.length, 1);
  let req = requests.pop();
  do_check_eq(req._method, "GET");
  do_check_eq(req._queryString, cohort ? "/" + cohort : "");
}

function promiseGlobalMetadata() {
  return new Promise(resolve => Task.spawn(function* () {
    let path = OS.Path.join(OS.Constants.Path.profileDir, "search-metadata.json");
    let bytes = yield OS.File.read(path);
    resolve(JSON.parse(new TextDecoder().decode(bytes))["[global]"]);
  }));
}

function promiseSaveGlobalMetadata(globalData) {
  return new Promise(resolve => Task.spawn(function* () {
    let path = OS.Path.join(OS.Constants.Path.profileDir, "search-metadata.json");
    let bytes = yield OS.File.read(path);
    let data = JSON.parse(new TextDecoder().decode(bytes));
    data["[global]"] = globalData;
    yield OS.File.writeAtomic(path,
                              new TextEncoder().encode(JSON.stringify(data)));
    resolve();
  }));
}

let forceExpiration = Task.async(function* () {
  let metadata = yield promiseGlobalMetadata();

  
  metadata.searchdefaultexpir = Date.now() - 1000;
  yield promiseSaveGlobalMetadata(metadata);
});

add_task(function* no_request_if_prefed_off() {
  
  Services.prefs.setBoolPref("browser.search.geoSpecificDefaults", false);
  yield asyncInit();
  checkNoRequest();

  
  do_check_eq(Services.search.currentEngine.name, getDefaultEngineName(false));

  
  
  let commitPromise = promiseAfterCommit();
  Services.search.currentEngine = Services.search.getEngineByName(kTestEngineName);
  Services.search.resetToOriginalDefaultEngine();
  yield commitPromise;

  
  let metadata = yield promiseGlobalMetadata();
  do_check_eq(typeof metadata.searchdefaultexpir, "undefined");
  do_check_eq(typeof metadata.searchdefault, "undefined");
  do_check_eq(typeof metadata.searchdefaulthash, "undefined");

  Services.prefs.setBoolPref("browser.search.geoSpecificDefaults", true);
});

add_task(function* should_get_geo_defaults_only_once() {
  
  
  let commitPromise = promiseAfterCommit();
  
  do_check_true(Services.prefs.prefHasUserValue("browser.search.countryCode"));
  do_check_eq(Services.prefs.getCharPref("browser.search.countryCode"), "FR");
  yield asyncReInit();
  checkRequest();
  do_check_eq(Services.search.currentEngine.name, kTestEngineName);
  yield commitPromise;

  
  let metadata = yield promiseGlobalMetadata();
  do_check_eq(typeof metadata.searchdefaultexpir, "number");
  do_check_true(metadata.searchdefaultexpir > Date.now());
  do_check_eq(typeof metadata.searchdefault, "string");
  do_check_eq(metadata.searchdefault, "Test search engine");
  do_check_eq(typeof metadata.searchdefaulthash, "string");
  do_check_eq(metadata.searchdefaulthash.length, 44);

  
  yield asyncReInit();
  checkNoRequest();
  do_check_eq(Services.search.currentEngine.name, kTestEngineName);
});

add_task(function* should_request_when_countryCode_not_set() {
  Services.prefs.clearUserPref("browser.search.countryCode");
  let commitPromise = promiseAfterCommit();
  yield asyncReInit();
  checkRequest();
  yield commitPromise;
});

add_task(function* should_recheck_if_interval_expired() {
  yield forceExpiration();

  let commitPromise = promiseAfterCommit();
  let date = Date.now();
  yield asyncReInit();
  checkRequest();
  yield commitPromise;

  
  let metadata = yield promiseGlobalMetadata();
  do_check_eq(typeof metadata.searchdefaultexpir, "number");
  do_check_true(metadata.searchdefaultexpir >= date + kYearInSeconds * 1000);
  do_check_true(metadata.searchdefaultexpir < date + (kYearInSeconds + 3600) * 1000);
});

add_task(function* should_recheck_when_broken_hash() {
  
  
  
  

  let metadata = yield promiseGlobalMetadata();

  
  let hash = metadata.searchdefaulthash;
  metadata.searchdefaulthash = "broken";
  yield promiseSaveGlobalMetadata(metadata);

  let commitPromise = promiseAfterCommit();
  let reInitPromise = asyncReInit();

  
  
  
  
  
  
  do_check_false(Services.search.isInitialized)
  do_check_eq(Services.search.currentEngine.name, getDefaultEngineName(false));
  do_check_true(Services.search.isInitialized)

  yield reInitPromise;
  checkRequest();
  yield commitPromise;

  
  metadata = yield promiseGlobalMetadata();
  do_check_eq(typeof metadata.searchdefaulthash, "string");
  do_check_eq(metadata.searchdefaulthash, hash);

  
  do_check_eq(Services.search.currentEngine.name, getDefaultEngineName(false));

  
  
  yield asyncReInit();
  checkNoRequest();
  do_check_eq(Services.search.currentEngine.name, kTestEngineName);
});

add_task(function* should_remember_cohort_id() {
  
  const cohortPref = "browser.search.cohort";
  do_check_eq(Services.prefs.getPrefType(cohortPref), Services.prefs.PREF_INVALID);

  
  let cohort = gServerCohort = "xpcshell";

  
  yield forceExpiration();
  let commitPromise = promiseAfterCommit();
  yield asyncReInit();
  checkRequest();
  yield commitPromise;

  
  do_check_eq(Services.prefs.getPrefType(cohortPref), Services.prefs.PREF_STRING);
  do_check_eq(Services.prefs.getCharPref(cohortPref), cohort);

  
  gServerCohort = "";

  
  
  yield forceExpiration();
  commitPromise = promiseAfterCommit();
  yield asyncReInit();
  checkRequest(cohort);
  yield commitPromise;
  do_check_eq(Services.prefs.getPrefType(cohortPref), Services.prefs.PREF_INVALID);
});

add_task(function* should_retry_after_failure() {
  let defaultBranch = Services.prefs.getDefaultBranch(BROWSER_SEARCH_PREF);
  let originalUrl = defaultBranch.getCharPref(kUrlPref);
  defaultBranch.setCharPref(kUrlPref, originalUrl.replace("defaults", "fail"));

  
  yield forceExpiration();
  yield asyncReInit();
  checkRequest();

  
  
  yield asyncReInit();
  checkRequest();
});

add_task(function* should_honor_retry_after_header() {
  let defaultBranch = Services.prefs.getDefaultBranch(BROWSER_SEARCH_PREF);
  let originalUrl = defaultBranch.getCharPref(kUrlPref);
  defaultBranch.setCharPref(kUrlPref, originalUrl.replace("fail", "unavailable"));

  
  yield forceExpiration();
  let date = Date.now();
  let commitPromise = promiseAfterCommit();
  yield asyncReInit();
  checkRequest();
  yield commitPromise;

  
  let metadata = yield promiseGlobalMetadata();
  do_check_eq(typeof metadata.searchdefaultexpir, "number");
  do_check_true(metadata.searchdefaultexpir >= date + kDayInSeconds * 1000);
  do_check_true(metadata.searchdefaultexpir < date + (kDayInSeconds + 3600) * 1000);

  
  yield asyncReInit();
  checkNoRequest();
});
