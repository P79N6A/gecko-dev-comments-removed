










const kGuidAnnotationName = "sync/guid";
const kExpectedAnnotations = 5;
const kExpectedValidGuids = 2;





var gItemGuid = [];
var gItemId = [];
var gPlaceGuid = [];
var gPlaceId = [];









function isValidGuid(aGuid)
{
  return /^[a-zA-Z0-9\-_]{12}$/.test(aGuid);
}




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
  do_check_false(db.indexExists("moz_places_guid_uniqueindex"));

  
  stmt = db.createStatement(
    "SELECT content AS guid, item_id "
  + "FROM moz_items_annos "
  + "WHERE anno_attribute_id = ( "
  +   "SELECT id "
  +   "FROM moz_anno_attributes "
  +   "WHERE name = :attr_name "
  + ") "
  );
  stmt.params.attr_name = kGuidAnnotationName;
  while (stmt.executeStep()) {
    gItemGuid.push(stmt.row.guid);
    gItemId.push(stmt.row.item_id)
  }
  do_check_eq(gItemGuid.length, gItemId.length);
  do_check_eq(gItemGuid.length, kExpectedAnnotations);
  stmt.finalize();

  
  stmt = db.createStatement(
    "SELECT content AS guid, place_id "
  + "FROM moz_annos "
  + "WHERE anno_attribute_id = ( "
  +   "SELECT id "
  +   "FROM moz_anno_attributes "
  +   "WHERE name = :attr_name "
  + ") "
  );
  stmt.params.attr_name = kGuidAnnotationName;
  while (stmt.executeStep()) {
    gPlaceGuid.push(stmt.row.guid);
    gPlaceId.push(stmt.row.place_id)
  }
  do_check_eq(gPlaceGuid.length, gPlaceId.length);
  do_check_eq(gPlaceGuid.length, kExpectedAnnotations);
  stmt.finalize();

  
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

function test_bookmark_guid_annotation_imported()
{
  
  let stmt = DBConn().createStatement(
    "SELECT id "
  + "FROM moz_bookmarks "
  + "WHERE guid = :guid "
  + "AND id = :item_id "
  );
  let validGuids = 0;
  let seenGuids = [];
  for (let i = 0; i < gItemGuid.length; i++) {
    let guid = gItemGuid[i];
    stmt.params.guid = guid;
    stmt.params.item_id = gItemId[i];

    
    
    let valid = isValidGuid(guid) && seenGuids.indexOf(guid) == -1;
    seenGuids.push(guid);

    if (valid) {
      validGuids++;
      do_check_true(stmt.executeStep());
    }
    else {
      do_check_false(stmt.executeStep());
    }
    stmt.reset();
  }
  do_check_eq(validGuids, kExpectedValidGuids);
  stmt.finalize();

  run_next_test();
}

function test_bookmark_guid_annotation_removed()
{
  let stmt = DBConn().createStatement(
    "SELECT COUNT(1) "
  + "FROM moz_items_annos "
  + "WHERE anno_attribute_id = ( "
  +   "SELECT id "
  +   "FROM moz_anno_attributes "
  +   "WHERE name = :attr_name "
  + ") "
  );
  stmt.params.attr_name = kGuidAnnotationName;
  do_check_true(stmt.executeStep());
  do_check_eq(stmt.getInt32(0), 0);
  stmt.finalize();

  run_next_test();
}

function test_moz_places_guid_exists()
{
  
  let stmt = DBConn().createStatement(
    "SELECT guid "
  + "FROM moz_places "
  );
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

function test_place_guid_annotation_imported()
{
  
  let stmt = DBConn().createStatement(
    "SELECT id "
  + "FROM moz_places "
  + "WHERE guid = :guid "
  + "AND id = :item_id "
  );
  let validGuids = 0;
  let seenGuids = [];
  for (let i = 0; i < gPlaceGuid.length; i++) {
    let guid = gPlaceGuid[i];
    stmt.params.guid = guid;
    stmt.params.item_id = gPlaceId[i];

    
    
    let valid = isValidGuid(guid) && seenGuids.indexOf(guid) == -1;
    seenGuids.push(guid);

    if (valid) {
      validGuids++;
      do_check_true(stmt.executeStep());
    }
    else {
      do_check_false(stmt.executeStep());
    }
    stmt.reset();
  }
  do_check_eq(validGuids, kExpectedValidGuids);
  stmt.finalize();

  run_next_test();
}

function test_place_guid_annotation_removed()
{
  let stmt = DBConn().createStatement(
    "SELECT COUNT(1) "
  + "FROM moz_annos "
  + "WHERE anno_attribute_id = ( "
  +   "SELECT id "
  +   "FROM moz_anno_attributes "
  +   "WHERE name = :attr_name "
  + ") "
  );
  stmt.params.attr_name = kGuidAnnotationName;
  do_check_true(stmt.executeStep());
  do_check_eq(stmt.getInt32(0), 0);
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
  do_check_true(db.indexExists("moz_places_guid_uniqueindex"));

  do_check_eq(db.schemaVersion, 11);

  db.close();
  run_next_test();
}




[
  test_initial_state,
  test_moz_bookmarks_guid_exists,
  test_bookmark_guids_non_null,
  test_bookmark_guid_annotation_imported,
  test_bookmark_guid_annotation_removed,
  test_moz_places_guid_exists,
  test_place_guids_non_null,
  test_place_guid_annotation_imported,
  test_place_guid_annotation_removed,
  test_final_state,
].forEach(add_test);

function run_test()
{
  setPlacesDatabase("places_v10.sqlite");
  run_next_test();
}
