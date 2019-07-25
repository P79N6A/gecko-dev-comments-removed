





function run_test() {
  
  let (dbFile = gProfD.clone()) {
    dbFile.append("places.sqlite");
    do_check_false(dbFile.exists());
  }
  let (dbFile = gProfD.clone()) {
    dbFile.append("places.sqlite.corrupt");
    do_check_false(dbFile.exists());
  }

  let file = do_get_file("default.sqlite");
  file.copyToFollowingLinks(gProfD, "places.sqlite");
  file = gProfD.clone().append("places.sqlite");

  
  let db = Services.storage.openUnsharedDatabase(file);
  db.executeSimpleSQL("CREATE TABLE test (id INTEGER PRIMARY KEY)");
  db.close();

  Services.prefs.setBoolPref("places.database.replaceOnStartup", true);
  do_check_eq(PlacesUtils.history.databaseStatus,
              PlacesUtils.history.DATABASE_STATUS_CORRUPT);

  let (dbFile = gProfD.clone()) {
    dbFile.append("places.sqlite");
    do_check_true(dbFile.exists());

    
    let db = Services.storage.openUnsharedDatabase(file);
    try {
      db.executeSimpleSQL("DELETE * FROM test");
      do_throw("The new database should not have our unique content");
    } catch(ex) {}
    db.close();
  }
  let (dbFile = gProfD.clone()) {
    dbFile.append("places.sqlite.corrupt");
    do_check_true(dbFile.exists());
  }
};
