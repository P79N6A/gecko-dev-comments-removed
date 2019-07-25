











function test_initial_state()
{
  
  
  let dbFile = gProfD.clone();
  dbFile.append(kDBName);
  let db = Services.storage.openUnsharedDatabase(dbFile);

  let stmt = db.createStatement("PRAGMA journal_mode");
  do_check_true(stmt.executeStep());
  
  
  do_check_neq(stmt.getString(0).toLowerCase(), "wal");
  stmt.finalize();

  do_check_true(db.indexExists("moz_bookmarks_guid_uniqueindex"));
  do_check_true(db.indexExists("moz_places_guid_uniqueindex"));

  
  stmt = db.createStatement(
    "SELECT COUNT(1) "
  + "FROM moz_bookmarks "
  + "WHERE guid IS NULL "
  );
  do_check_true(stmt.executeStep());
  do_check_neq(stmt.getInt32(0), 0);
  stmt.finalize();

  
  stmt = db.createStatement(
    "SELECT COUNT(1) "
  + "FROM moz_places "
  + "WHERE guid IS NULL "
  );
  do_check_true(stmt.executeStep());
  do_check_neq(stmt.getInt32(0), 0);
  stmt.finalize();

  
  do_check_eq(db.schemaVersion, 10);

  db.close();
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

function test_place_guids_non_null()
{
  
  
  let stmt = DBConn().createStatement(
    "SELECT COUNT(1) "
  + "FROM moz_places "
  );
  do_check_true(stmt.executeStep());
  do_check_neq(stmt.getInt32(0), 0);
  stmt.finalize();

  
  stmt = DBConn().createStatement(
    "SELECT guid "
  + "FROM moz_places "
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

  do_check_eq(db.schemaVersion, 11);

  db.close();
  run_next_test();
}




[
  test_initial_state,
  test_bookmark_guids_non_null,
  test_place_guids_non_null,
  test_final_state,
].forEach(add_test);

function run_test()
{
  setPlacesDatabase("places_v10_from_v11.sqlite");
  run_next_test();
}
