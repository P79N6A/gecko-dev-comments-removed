












let iter = tests();

function run_test()
{
  do_test_pending();
  iter.next();
}

function next_test()
{
  iter.next();
}

function tests()
{
  try {
  var testnum = 0;

  
  var testfile = do_get_file("formhistory_v999a.sqlite");
  var profileDir = dirSvc.get("ProfD", Ci.nsIFile);

  
  var destFile = profileDir.clone();
  destFile.append("formhistory.sqlite");
  if (destFile.exists())
    destFile.remove(false);

  testfile.copyTo(profileDir, "formhistory.sqlite");
  do_check_eq(999, getDBVersion(testfile));

  let checkZero = function(num) { do_check_eq(num, 0); next_test(); }
  let checkOne = function(num) { do_check_eq(num, 1); next_test(); }

  
  testnum++;
  
  yield countEntries(null, null, function(num) { do_check_true(num > 0); next_test(); });
  yield countEntries("name-A", "value-A", checkOne);
  yield countEntries("name-B", "value-B", checkOne);
  yield countEntries("name-C", "value-C1", checkOne);
  yield countEntries("name-C", "value-C2", checkOne);
  yield countEntries("name-E", "value-E", checkOne);

  
  do_check_eq(CURRENT_SCHEMA, FormHistory.schemaVersion);

  
  testnum++;
  
  yield countEntries("name-D", "value-D", checkZero);
  yield updateEntry("add", "name-D", "value-D", next_test);
  yield countEntries("name-D", "value-D", checkOne);
  yield updateEntry("remove", "name-D", "value-D", next_test);
  yield countEntries("name-D", "value-D", checkZero);

  } catch (e) {
    throw "FAILED in test #" + testnum + " -- " + e;
  }

  do_test_finished();
}
