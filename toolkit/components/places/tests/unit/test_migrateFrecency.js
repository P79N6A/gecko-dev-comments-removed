







































function _(msg) {
  dump(".-* DEBUG *-. " + msg + "\n");
}

function run_test() {
  _("Copy the history file with plenty of data to migrate");
  let dirSvc = Cc["@mozilla.org/file/directory_service;1"].
    getService(Ci.nsIProperties);
  let deleteMork = function() {
    let mork = dirSvc.get("ProfD", Ci.nsIFile);
    mork.append("history.dat");
    if (mork.exists())
      mork.remove(false);
  };

  deleteMork();
  let file = do_get_file("migrateFrecency.dat");
  file.copyTo(dirSvc.get("ProfD", Ci.nsIFile), "history.dat");

  _("Wait until places is done initializing to check migration");
  let places = null;
  const NS_PLACES_INIT_COMPLETE_TOPIC = "places-init-complete";
  let os = Cc["@mozilla.org/observer-service;1"].
    getService(Ci.nsIObserverService);
  let observer = {
    observe: function (subject, topic, data) {
      switch (topic) {
        case NS_PLACES_INIT_COMPLETE_TOPIC:
          _("Clean up after ourselves: remove observer and mork file");
          os.removeObserver(observer, NS_PLACES_INIT_COMPLETE_TOPIC);
          deleteMork();

          _("Now that places has migrated, check that it calculated frecencies");
          var stmt = places.DBConnection.createStatement(
              "SELECT COUNT(*) FROM moz_places_view WHERE frecency < 0");
          stmt.executeAsync({
              handleResult: function(results) {
                _("Should always get a result from COUNT(*)");
                let row = results.getNextRow();
                do_check_true(!!row);

                _("We should have no negative frecencies after migrate");
                do_check_eq(row.getResultByIndex(0), 0);
              },
              handleCompletion: do_test_finished,
              handleError: do_throw,
            });
          stmt.finalize();
          break;
      }
    },
  };
  os.addObserver(observer, NS_PLACES_INIT_COMPLETE_TOPIC, false);

  _("Start places to make it migrate");
  places = Cc["@mozilla.org/browser/nav-history-service;1"].
    getService(Ci.nsPIPlacesDatabase);

  do_test_pending();
}
