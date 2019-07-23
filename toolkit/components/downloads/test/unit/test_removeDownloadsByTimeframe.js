














































let dm = Cc["@mozilla.org/download-manager;1"].
         getService(Ci.nsIDownloadManager);

const START_TIME = Date.now() * 1000;
const END_TIME = START_TIME + (60 * 1000000); 

const DOWNLOAD_FINISHED = Ci.nsIDownloadManager.DOWNLOAD_FINISHED;
const DOWNLOAD_DOWNLOADING = Ci.nsIDownloadManager.DOWNLOAD_DOWNLOADING;

const REMOVED_TOPIC = "download-manager-remove-download";













let id = 1;
function add_download_to_db(aStartTimeInRange, aEndTimeInRange, aState)
{
  let db = dm.DBConnection;
  let stmt = db.createStatement(
    "INSERT INTO moz_downloads (id, startTime, endTime, state) " +
    "VALUES (:id, :startTime, :endTime, :state)"
  );
  stmt.params.id = id;
  stmt.params.startTime = aStartTimeInRange ? START_TIME + 1 : START_TIME - 1;
  stmt.params.endTime = aEndTimeInRange ? END_TIME - 1 : END_TIME + 1;
  stmt.params.state = aState;
  stmt.execute();

  return id++;
}









function check_existance(aIDs, aExpected)
{
  let db = dm.DBConnection;
  let stmt = db.createStatement(
    "SELECT * " +
    "FROM moz_downloads " +
    "WHERE id = :id"
  );

  let checkFunc = aExpected ? do_check_true : do_check_false;
  for (let i = 0; i < aIDs.length; i++) {
    stmt.params.id = aIDs[i];
    checkFunc(stmt.executeStep());
    stmt.reset();
  }
}




function test_download_start_in_range()
{
  let id = add_download_to_db(true, false, DOWNLOAD_FINISHED);
  dm.removeDownloadsByTimeframe(START_TIME, END_TIME);
  check_existance([id], false);
}

function test_download_end_in_range()
{
  let id = add_download_to_db(false, true, DOWNLOAD_FINISHED);
  dm.removeDownloadsByTimeframe(START_TIME, END_TIME);
  check_existance([id], true);
}

function test_multiple_downloads_in_range()
{
  let ids = [];
  ids.push(add_download_to_db(true, false, DOWNLOAD_FINISHED));
  ids.push(add_download_to_db(true, false, DOWNLOAD_FINISHED));
  dm.removeDownloadsByTimeframe(START_TIME, END_TIME);
  check_existance(ids, false);
}

function test_no_downloads_in_range()
{
  let ids = [];
  ids.push(add_download_to_db(false, true, DOWNLOAD_FINISHED));
  ids.push(add_download_to_db(false, true, DOWNLOAD_FINISHED));
  dm.removeDownloadsByTimeframe(START_TIME, END_TIME);
  check_existance(ids, true);
}

function test_active_download_in_range()
{
  let id = add_download_to_db(true, false, DOWNLOAD_DOWNLOADING);
  dm.removeDownloadsByTimeframe(START_TIME, END_TIME);
  check_existance([id], true);
}

function test_observer_dispatched()
{
  let observer = {
    notified: false,
    observe: function(aSubject, aTopic, aData)
    {
      this.notified = true;
      do_check_eq(aSubject, null);
      do_check_eq(aTopic, REMOVED_TOPIC);
      do_check_eq(aData, null);
    }
  };
  let os = Cc["@mozilla.org/observer-service;1"].
           getService(Ci.nsIObserverService);
  os.addObserver(observer, REMOVED_TOPIC, false);

  add_download_to_db(true, false, DOWNLOAD_FINISHED);
  dm.removeDownloadsByTimeframe(START_TIME, END_TIME);
  do_check_true(observer.notified);

  os.removeObserver(observer, REMOVED_TOPIC);
}

let tests = [
  test_download_start_in_range,
  test_download_end_in_range,
  test_multiple_downloads_in_range,
  test_no_downloads_in_range,
  test_active_download_in_range,
  test_observer_dispatched,
];

function run_test()
{
  for (let i = 0; i < tests.length; i++) {
    dm.cleanUp();
    tests[i]();
  }
}
