











function test_initial_state()
{
  
  
  let dbFile = gProfD.clone();
  dbFile.append(kDBName);
  let db = Services.storage.openUnsharedDatabase(dbFile);

  let stmt = db.createStatement("PRAGMA journal_mode");
  do_check_true(stmt.executeStep());
  
  do_check_neq(stmt.getString(0).toLowerCase(), "wal");
  stmt.finalize();

  do_check_false(db.indexExists("moz_bookmarks_guid_uniqueindex"));

  do_check_eq(db.schemaVersion, 10);

  db.close();
  run_next_test();
}

function test_moz_bookmarks_guid_exists()
{
  
  let stmt = DBConn().createStatement(
    "SELECT guid "
  + "FROM moz_bookmarks "
  );
  stmt.finalize();

  run_next_test();
}

function test_bookmark_guids_non_null()
{
  
  let stmt = DBConn().createStatement(
    "SELECT COUNT(1) "
  + "FROM moz_bookmarks "
  );
  do_check_true(stmt.executeStep());
  do_check_neq(stmt.getInt32(0), 0);
  stmt.finalize();

  
  stmt = DBConn().createStatement(
    "SELECT guid "
  + "FROM moz_bookmarks "
  + "WHERE guid IS NULL "
  );
  do_check_false(stmt.executeStep());
  stmt.finalize();
  run_next_test();
}

function test_final_state()
{
  
  
  let dbFile = gProfD.clone();
  dbFile.append(kDBName);
  let db = Services.storage.openUnsharedDatabase(dbFile);

  let (stmt = db.createStatement("PRAGMA journal_mode")) {
    do_check_true(stmt.executeStep());
    
    do_check_eq(stmt.getString(0).toLowerCase(), "wal");
    stmt.finalize();
  }

  do_check_true(db.indexExists("moz_bookmarks_guid_uniqueindex"));

  do_check_eq(db.schemaVersion, 11);

  db.close();
  run_next_test();
}




let tests = [
  test_initial_state,
  test_moz_bookmarks_guid_exists,
  test_bookmark_guids_non_null,
  test_final_state,
];
let index = 0;

function run_next_test()
{
  function _run_next_test() {
    if (index < tests.length) {
      do_test_pending();
      print("TEST-INFO | " + _TEST_FILE + " | Starting " + tests[index].name);

      
      try {
        tests[index++]();
      }
      catch (e) {
        do_throw(e);
      }
    }

    do_test_finished();
  }

  
  do_execute_soon(_run_next_test);
}

function run_test()
{
  setPlacesDatabase("places_v10.sqlite");
  do_test_pending();
  run_next_test();
}
