












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

  
  var testfile = do_get_file("formhistory_v999b.sqlite");
  var profileDir = dirSvc.get("ProfD", Ci.nsIFile);

  
  var destFile = profileDir.clone();
  destFile.append("formhistory.sqlite");
  if (destFile.exists())
    destFile.remove(false);

  var bakFile = profileDir.clone();
  bakFile.append("formhistory.sqlite.corrupt");
  if (bakFile.exists())
    bakFile.remove(false);

  testfile.copyTo(profileDir, "formhistory.sqlite");
  do_check_eq(999, getDBVersion(testfile));

  let checkZero = function(num) { do_check_eq(num, 0); next_test(); }
  let checkOne = function(num) { do_check_eq(num, 1); next_test(); }

  
  testnum++;

  
  
  do_check_false(bakFile.exists());
  
  yield countEntries("", "", next_test);

  do_check_true(bakFile.exists());
  bakFile.remove(false);

  
  testnum++;
  
  yield countEntries(null, null, function(num) { do_check_false(num); next_test(); });
  yield countEntries("name-A", "value-A", checkZero);
  
  do_check_eq(CURRENT_SCHEMA, FormHistory.schemaVersion);

  
  testnum++;
  
  yield updateEntry("add", "name-A", "value-A", next_test);
  yield countEntries(null, null, checkOne);
  yield countEntries("name-A", "value-A", checkOne);

  
  testnum++;
  
  yield updateEntry("remove", "name-A", "value-A", next_test);
  yield countEntries(null, null, checkZero);
  yield countEntries("name-A", "value-A", checkZero);

  } catch (e) {
    throw "FAILED in test #" + testnum + " -- " + e;
  }

  do_test_finished();
}
