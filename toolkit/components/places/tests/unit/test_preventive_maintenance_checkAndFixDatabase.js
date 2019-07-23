





































 




Components.utils.import("resource://gre/modules/PlacesDBUtils.jsm");

function run_test() {
  do_test_pending();
  PlacesDBUtils.checkAndFixDatabase(function(aLog) {
    
    do_check_eq(aLog[1], "ok");

    
    const goodMsgs = ["INTEGRITY", "EXPIRE", "VACUUM", "CLEANUP", "STATS"];
    const badMsgs = ["BACKUP", "REINDEX"];

    aLog.forEach(function (aLogMsg) {
      print (aLogMsg);
      do_check_eq(badMsgs.indexOf(aLogMsg), -1);
      let index = goodMsgs.indexOf(aLogMsg);
      if (index != -1)
        goodMsgs.splice(index, 1);
    });

    
    do_check_eq(goodMsgs.length, 0);

    do_test_finished();
  });
}
