const {utils: Cu} = Components;
Cu.import("resource://gre/modules/TelemetryArchive.jsm");
Cu.import("resource://testing-common/Assert.jsm");
Cu.import("resource://gre/modules/Task.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/TelemetryController.jsm");

this.EXPORTED_SYMBOLS = [
  "TelemetryArchiveTesting",
];

function checkForProperties(ping, expected) {
  for (let [props, val] of expected) {
    let test = ping;
    for (let prop of props) {
      test = test[prop];
      if (test === undefined) {
        return false;
      }
    }
    if (test !== val) {
      return false;
    }
  }
  return true;
}






function Checker() {
}
Checker.prototype = {
  promiseInit: function() {
    this._pingMap = new Map();
    return TelemetryArchive.promiseArchivedPingList().then((plist) => {
      for (let ping of plist) {
        this._pingMap.set(ping.id, ping);
      }
    });
  },

  











  promiseFindPing: Task.async(function*(type, expected) {
    let candidates = [];
    let plist = yield TelemetryArchive.promiseArchivedPingList();
    for (let ping of plist) {
      if (this._pingMap.has(ping.id)) {
        continue;
      }
      if (ping.type == type) {
        candidates.push(ping);
      }
    }

    for (let candidate of candidates) {
      let ping = yield TelemetryArchive.promiseArchivedPingById(candidate.id);
      if (checkForProperties(ping, expected)) {
        return ping;
      }
    }
    return null;
  }),
};

const TelemetryArchiveTesting = {
  setup: function() {
    Services.prefs.setCharPref("toolkit.telemetry.log.level", "Trace");
    Services.prefs.setBoolPref("toolkit.telemetry.archive.enabled", true);
  },

  Checker: Checker,
};
