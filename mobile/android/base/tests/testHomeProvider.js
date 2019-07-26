




const { utils: Cu } = Components;

Cu.import("resource://gre/modules/HomeProvider.jsm");
Cu.import("resource://gre/modules/osfile.jsm");
Cu.import("resource://gre/modules/Sqlite.jsm");
Cu.import("resource://gre/modules/Task.jsm");

const TEST_DATASET_ID = "test-dataset-id";
const TEST_URL = "http://test.com";

const DB_PATH = OS.Path.join(OS.Constants.Path.profileDir, "home.sqlite");

add_task(function test_save_and_delete() {
  
  let storage = HomeProvider.getStorage(TEST_DATASET_ID);
  yield storage.save([{ url: TEST_URL }]);

  
  let db = yield Sqlite.openConnection({ path: DB_PATH });

  
  do_check_true(yield db.tableExists("items"));

  
  let result = yield db.execute("SELECT * FROM items", null, function onRow(row){
    do_check_eq(row.getResultByName("dataset_id"), TEST_DATASET_ID);
    do_check_eq(row.getResultByName("url"), TEST_URL);
  });

  
  yield storage.deleteAll();

  
  let result = yield db.execute("SELECT * FROM items");
  do_check_eq(result.length, 0);

  db.close();
});

run_next_test();
