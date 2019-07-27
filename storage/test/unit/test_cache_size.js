



















function check_size(dbOpener, file, pageSize, expectedCacheSize)
{
  
  let db = dbOpener(file);
  db.executeSimpleSQL("PRAGMA page_size = " + pageSize);

  
  let stmt = db.createStatement("PRAGMA page_size");
  do_check_true(stmt.executeStep());
  do_check_eq(stmt.row.page_size, pageSize);
  stmt.finalize();

  
  db.executeSimpleSQL("CREATE TABLE test ( id INTEGER PRIMARY KEY )");

  
  db.close();
  db = dbOpener(file);

  
  stmt = db.createStatement("PRAGMA cache_size");
  do_check_true(stmt.executeStep());
  do_check_eq(stmt.row.cache_size, expectedCacheSize);
  stmt.finalize();
}

function new_file(name)
{
  let file = dirSvc.get("ProfD", Ci.nsIFile);
  file.append(name + ".sqlite");
  do_check_false(file.exists());
  return file;
}

function run_test()
{
  const kExpectedCacheSize = -2048; 

  let pageSizes = [
    1024,
    4096,
    32768,
  ];

  for (let i = 0; i < pageSizes.length; i++) {
    let pageSize = pageSizes[i];
    check_size(getDatabase,
               new_file("shared" + pageSize), pageSize, kExpectedCacheSize);
    check_size(getService().openUnsharedDatabase,
               new_file("unshared" + pageSize), pageSize, kExpectedCacheSize);
  }
}
