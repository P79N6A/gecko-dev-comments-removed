




const { utils: Cu } = Components;

Cu.import("resource://gre/modules/HomeProvider.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Services.jsm");
Cu.import("resource://gre/modules/Sqlite.jsm");
Cu.import("resource://gre/modules/Task.jsm");

const TEST_DATASET_ID = "test-dataset-id";
const TEST_URL = "http://test.com";
const TEST_TITLE = "Test";

const PREF_SYNC_CHECK_INTERVAL_SECS = "home.sync.checkIntervalSecs";
const TEST_INTERVAL_SECS = 1;

const DB_PATH = OS.Path.join(OS.Constants.Path.profileDir, "home.sqlite");

add_test(function test_request_sync() {
  
  let success = HomeProvider.requestSync(TEST_DATASET_ID, function callback(datasetId) {
    do_check_eq(datasetId, TEST_DATASET_ID);
  });

  do_check_true(success);
  run_next_test();
});

add_test(function test_periodic_sync() {
  do_register_cleanup(function cleanup() {
    Services.prefs.clearUserPref(PREF_SYNC_CHECK_INTERVAL_SECS);
    HomeProvider.removePeriodicSync(TEST_DATASET_ID);
  });

  
  Services.prefs.setIntPref(PREF_SYNC_CHECK_INTERVAL_SECS, TEST_INTERVAL_SECS);

  HomeProvider.addPeriodicSync(TEST_DATASET_ID, TEST_INTERVAL_SECS, function callback(datasetId) {
    do_check_eq(datasetId, TEST_DATASET_ID);
    run_next_test();
  });
});

add_task(function test_save_and_delete() {
  
  let storage = HomeProvider.getStorage(TEST_DATASET_ID);
  yield storage.save([{ title: TEST_TITLE, url: TEST_URL }]);

  
  let db = yield Sqlite.openConnection({ path: DB_PATH });

  
  do_check_true(yield db.tableExists("items"));

  
  let result = yield db.execute("SELECT * FROM items", null, function onRow(row){
    do_check_eq(row.getResultByName("dataset_id"), TEST_DATASET_ID);
    do_check_eq(row.getResultByName("url"), TEST_URL);
  });

  
  yield storage.deleteAll();

  
  result = yield db.execute("SELECT * FROM items");
  do_check_eq(result.length, 0);

  db.close();
});

add_task(function test_row_validation() {
  
  let storage = HomeProvider.getStorage(TEST_DATASET_ID);

  let invalidRows = [
    { url: "url" },
    { title: "title" },
    { description: "description" },
    { image_url: "image_url" }
  ];

  
  for (let row of invalidRows) {
    try {
      yield storage.save([row]);
    } catch (e if e instanceof HomeProvider.ValidationError) {
      
    }
  }

  
  let db = yield Sqlite.openConnection({ path: DB_PATH });

  
  let result = yield db.execute("SELECT * FROM items");
  do_check_eq(result.length, 0);

  db.close();
});

add_task(function test_save_transaction() {
  
  let storage = HomeProvider.getStorage(TEST_DATASET_ID);

  
  let rows = [
    { title: TEST_TITLE, url: TEST_URL },
    { image_url: "image_url" }
  ];

  
  try {
    yield storage.save(rows);
  } catch (e if e instanceof HomeProvider.ValidationError) {
    
  }

  
  let db = yield Sqlite.openConnection({ path: DB_PATH });

  
  let result = yield db.execute("SELECT * FROM items");
  do_check_eq(result.length, 0);

  db.close();
});

run_next_test();
