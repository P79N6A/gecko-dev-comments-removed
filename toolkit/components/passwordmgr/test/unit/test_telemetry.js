








"use strict";




const MS_PER_DAY = 24 * 60 * 60 * 1000;




const gReferenceTimeMs = new Date("2000-01-01T00:00:00").getTime();




let daysBeforeMs = days => gReferenceTimeMs - (days + 0.5) * MS_PER_DAY;









const StatisticsTestData = [
  {
    timeLastUsed: daysBeforeMs(0),
  },
  {
    timeLastUsed: daysBeforeMs(1),
  },
  {
    timeLastUsed: daysBeforeMs(7),
    formSubmitURL: null,
    httpRealm: "The HTTP Realm",
  },
  {
    username: "",
    timeLastUsed: daysBeforeMs(7),
  },
  {
    username: "",
    timeLastUsed: daysBeforeMs(30),
  },
  {
    username: "",
    timeLastUsed: daysBeforeMs(31),
  },
  {
    timeLastUsed: daysBeforeMs(365),
  },
  {
    username: "",
    timeLastUsed: daysBeforeMs(366),
  },
  {
    
    timeLastUsed: daysBeforeMs(-1),
  },
  {
    timeLastUsed: daysBeforeMs(1000),
  },
];





function triggerStatisticsCollection() {
  Services.obs.notifyObservers(null, "gather-telemetry", "" + gReferenceTimeMs);
}





function testHistogram(histogramId, expectedNonZeroRanges) {
  let snapshot = Services.telemetry.getHistogramById(histogramId).snapshot();

  
  let actualNonZeroRanges = {};
  for (let [index, range] of snapshot.ranges.entries()) {
    let value = snapshot.counts[index];
    if (value > 0) {
      actualNonZeroRanges[range] = value;
    }
  }

  
  do_print("Testing histogram: " + histogramId);
  do_check_eq(JSON.stringify(actualNonZeroRanges),
              JSON.stringify(expectedNonZeroRanges));
}








add_task(function test_initialize() {
  let oldCanRecord = Services.telemetry.canRecordExtended;
  Services.telemetry.canRecordExtended = true;
  do_register_cleanup(function () {
    Services.telemetry.canRecordExtended = oldCanRecord;
  });

  let uniqueNumber = 1;
  for (let loginModifications of StatisticsTestData) {
    loginModifications.hostname = `http://${uniqueNumber++}.example.com`;
    Services.logins.addLogin(TestData.formLogin(loginModifications));
  }
});




add_task(function test_logins_statistics() {
  
  for (let repeating of [false, true]) {
    triggerStatisticsCollection();

    
    testHistogram("PWMGR_NUM_SAVED_PASSWORDS",
                  { 10: 1 });

    
    testHistogram("PWMGR_NUM_HTTPAUTH_PASSWORDS",
                  { 1: 1 });

    
    
    testHistogram("PWMGR_LOGIN_LAST_USED_DAYS",
                  { 0: 1, 1: 1, 7: 2, 29: 2, 356: 2, 750: 1 });

    
    
    testHistogram("PWMGR_USERNAME_PRESENT",
                  { 0: 4, 1: 6 });
  }
});





add_task(function test_disabledHosts_statistics() {
  
  
  Services.logins.setLoginSavingEnabled("http://www.example.com", false);
  triggerStatisticsCollection();
  testHistogram("PWMGR_BLOCKLIST_NUM_SITES", { 1: 1 });

  Services.logins.setLoginSavingEnabled("http://www.example.com", true);
  triggerStatisticsCollection();
  testHistogram("PWMGR_BLOCKLIST_NUM_SITES", { 0: 1 });
});




add_task(function test_settings_statistics() {
  let oldRememberSignons = Services.prefs.getBoolPref("signon.rememberSignons");
  do_register_cleanup(function () {
    Services.prefs.setBoolPref("signon.rememberSignons", oldRememberSignons);
  });

  
  for (let remember of [false, true, false, true]) {
    
    Services.prefs.setBoolPref("signon.rememberSignons", remember);

    triggerStatisticsCollection();

    
    testHistogram("PWMGR_SAVING_ENABLED", remember ? { 1: 1 } : { 0: 1 });
  }
});
