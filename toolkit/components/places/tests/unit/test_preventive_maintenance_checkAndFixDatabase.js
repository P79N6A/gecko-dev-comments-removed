





































 




Components.utils.import("resource://gre/modules/PlacesDBUtils.jsm");

const FINISHED_MAINTENANCE_NOTIFICATION_TOPIC = "places-maintenance-finished";

let os = Cc["@mozilla.org/observer-service;1"].
         getService(Ci.nsIObserverService);
let log = [];

let observer = {
  observe: function(aSubject, aTopic, aData) {
    if (aTopic == FINISHED_MAINTENANCE_NOTIFICATION_TOPIC) {
      
      do_check_eq(log[1], "ok");

      
      const goodMsgs = ["INTEGRITY", "VACUUM", "CLEANUP", "STATS"];
      const badMsgs = ["BACKUP", "REINDEX"];
      log.forEach(function (aLogMsg) {
        do_check_eq(badMsgs.indexOf(aLogMsg), -1);
        let index = goodMsgs.indexOf(aLogMsg);
        if (index != -1)
          goodMsgs.splice(index, 1);
      });
      
      do_check_eq(goodMsgs.length, 0);

      do_test_finished();
    }
  }
}
os.addObserver(observer, FINISHED_MAINTENANCE_NOTIFICATION_TOPIC, false);

function run_test() {
  do_test_pending();
  log = PlacesDBUtils.checkAndFixDatabase().split("\n");
}
