


"use strict";

add_task(function* test_healthreport_search_recording() {
  try {
    let cm = Cc["@mozilla.org/categorymanager;1"].getService(Ci.nsICategoryManager);
    cm.getCategoryEntry("healthreport-js-provider-default", "SearchesProvider");
  } catch (ex) {
    
    ok(true, "Firefox Health Report is not enabled.");
    return;
  }

  let reporter = Cc["@mozilla.org/datareporting/service;1"]
                   .getService()
                   .wrappedJSObject
                   .healthReporter;
  ok(reporter, "Health Reporter available.");
  yield reporter.onInit();
  let provider = reporter.getProvider("org.mozilla.searches");
  ok(provider, "Searches provider is available.");
  let m = provider.getMeasurement("counts", 3);

  let data = yield m.getValues();
  let now = new Date();
  let oldCount = 0;

  
  
  
  let defaultEngineID = "google";

  let field = defaultEngineID + ".urlbar";

  if (data.days.hasDay(now)) {
    let day = data.days.getDay(now);
    if (day.has(field)) {
      oldCount = day.get(field);
    }
  }

  let tab = gBrowser.addTab("about:blank");
  yield BrowserTestUtils.browserLoaded(tab.linkedBrowser);
  gBrowser.selectedTab = tab;

  let searchStr = "firefox health report";
  let expectedURL = Services.search.currentEngine.
                    getSubmission(searchStr, "", "keyword").uri.spec;

  
  let docLoadPromise = waitForDocLoadAndStopIt(expectedURL);

  
  gURLBar.value = searchStr;
  gURLBar.handleCommand();

  yield docLoadPromise;

  data = yield m.getValues();
  ok(data.days.hasDay(now), "We have a search measurement for today.");
  let day = data.days.getDay(now);
  ok(day.has(field), "Have a search count for the urlbar.");
  let newCount = day.get(field);
  is(newCount, oldCount + 1, "We recorded one new search.");

  
  let oldTelemetry = Services.prefs.getBoolPref("toolkit.telemetry.enabled");
  Services.prefs.setBoolPref("toolkit.telemetry.enabled", true);

  m = provider.getMeasurement("engines", 2);
  yield provider.collectDailyData();
  data = yield m.getValues();

  ok(data.days.hasDay(now), "Have engines data when Telemetry is enabled.");
  day = data.days.getDay(now);
  ok(day.has("default"), "We have default engine data.");
  is(day.get("default"), defaultEngineID, "The default engine is reported properly.");

  
  Services.prefs.setBoolPref("toolkit.telemetry.enabled", oldTelemetry);

  gBrowser.removeTab(tab);
});
