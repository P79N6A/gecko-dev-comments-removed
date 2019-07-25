





const kExpectedPageSize = 32768; 
const kExpectedCacheSize = 128; 

function check_size(db)
{
  var stmt = db.createStatement("PRAGMA page_size");
  stmt.executeStep();
  do_check_eq(stmt.getInt32(0), kExpectedPageSize);
  stmt.finalize();
  stmt = db.createStatement("PRAGMA cache_size");
  stmt.executeStep();
  do_check_eq(stmt.getInt32(0), kExpectedCacheSize);
  stmt.finalize();
}

function new_file(name)
{
  var file = dirSvc.get("ProfD", Ci.nsIFile);
  file.append(name + ".sqlite");
  do_check_false(file.exists());
  return file;
}

function run_test()
{
  check_size(getDatabase(new_file("shared32k")));
  check_size(getService().openUnsharedDatabase(new_file("unshared32k")));
}

