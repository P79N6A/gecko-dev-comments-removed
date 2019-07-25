





function check_size(db)
{
  var stmt = db.createStatement("PRAGMA page_size");
  stmt.executeStep();
  const expected_block_size = 32768; 
  do_check_eq(stmt.getInt32(0), expected_block_size);
  stmt.finalize();
}

function new_file(name)
{
  var file = dirSvc.get("ProfD", Ci.nsIFile);
  file.append(name + ".sqlite");
  do_check_false(file.exists());
}

function run_test()
{
  check_size(getDatabase(new_file("shared32k.sqlite")));
  check_size(getService().openUnsharedDatabase(new_file("unshared32k.sqlite")));
}

