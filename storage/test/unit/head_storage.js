




































const Ci = Components.interfaces;
const Cc = Components.classes;
const Cr = Components.results;

do_get_profile();
var dirSvc = Cc["@mozilla.org/file/directory_service;1"].
             getService(Ci.nsIProperties);

function getTestDB()
{
  var db = dirSvc.get("ProfD", Ci.nsIFile);
  db.append("test_storage.sqlite");
  return db;
}




function getCorruptDB()
{
  return do_get_file("corruptDB.sqlite");
}




function getFakeDB()
{
  return do_get_file("fakeDB.sqlite");
}

function cleanup()
{
  
  print("*** Storage Tests: Trying to close!");
  getOpenedDatabase().close();

  
  
  gDBConn = null;

  
  print("*** Storage Tests: Trying to remove file!");
  var dbFile = getTestDB();
  if (dbFile.exists())
    try { dbFile.remove(false); } catch(e) {  }
}





function asyncCleanup()
{
  let closed = false;

  
  print("*** Storage Tests: Trying to asyncClose!");
  getOpenedDatabase().asyncClose(function() { closed = true; });

  let curThread = Components.classes["@mozilla.org/thread-manager;1"]
                            .getService().currentThread;
  while (!closed)
    curThread.processNextEvent(true);

  
  
  gDBConn = null;

  
  print("*** Storage Tests: Trying to remove file!");
  var dbFile = getTestDB();
  if (dbFile.exists())
    try { dbFile.remove(false); } catch(e) {  }
}

function getService()
{
  return Cc["@mozilla.org/storage/service;1"].getService(Ci.mozIStorageService);
}

var gDBConn = null;











function getOpenedDatabase(unshared)
{
  if (!gDBConn) {
    gDBConn = getService()
              [unshared ? "openUnsharedDatabase" : "openDatabase"]
              (getTestDB());
  }
  return gDBConn;
}








function getDatabase(aFile)
{
  return getService().openDatabase(aFile);
}

function createStatement(aSQL)
{
  return getOpenedDatabase().createStatement(aSQL);
}








function createAsyncStatement(aSQL)
{
  return getOpenedDatabase().createAsyncStatement(aSQL);
}














function expectError(aErrorCode, aFunction)
{
  let exceptionCaught = false;
  try {
    aFunction();
  }
  catch(e) {
    if (e.result != aErrorCode) {
      do_throw("Got an exception, but the result code was not the expected " +
               "one.  Expected " + aErrorCode + ", got " + e.result);
    }
    exceptionCaught = true;
  }
  if (!exceptionCaught)
    do_throw(aFunction + " should have thrown an exception but did not!");
}












function verifyQuery(aSQLString, aBind, aResults)
{
  let stmt = getOpenedDatabase().createStatement(aSQLString);
  stmt.bindByIndex(0, aBind);
  try {
    do_check_true(stmt.executeStep());
    let nCols = stmt.numEntries;
    if (aResults.length != nCols)
      do_throw("Expected " + aResults.length + " columns in result but " +
               "there are only " + aResults.length + "!");
    for (let iCol = 0; iCol < nCols; iCol++) {
      let expectedVal = aResults[iCol];
      let valType = stmt.getTypeOfIndex(iCol);
      if (expectedVal === null) {
        do_check_eq(stmt.VALUE_TYPE_NULL, valType);
        do_check_true(stmt.getIsNull(iCol));
      }
      else if (typeof(expectedVal) == "number") {
        if (Math.floor(expectedVal) == expectedVal) {
          do_check_eq(stmt.VALUE_TYPE_INTEGER, valType);
          do_check_eq(expectedVal, stmt.getInt32(iCol));
        }
        else {
          do_check_eq(stmt.VALUE_TYPE_FLOAT, valType);
          do_check_eq(expectedVal, stmt.getDouble(iCol));
        }
      }
      else if (typeof(expectedVal) == "string") {
        do_check_eq(stmt.VALUE_TYPE_TEXT, valType);
        do_check_eq(expectedVal, stmt.getUTF8String(iCol));
      }
      else { 
        do_check_eq(stmt.VALUE_TYPE_BLOB, valType);
        let count = { value: 0 }, blob = { value: null };
        stmt.getBlob(iCol, count, blob);
        do_check_eq(count.value, expectedVal.length);
        for (let i = 0; i < count.value; i++) {
          do_check_eq(expectedVal[i], blob.value[i]);
        }
      }
    }
  }
  finally {
    stmt.finalize();
  }
}









function getTableRowCount(aTableName)
{
  var currentRows = 0;
  var countStmt = getOpenedDatabase().createStatement(
    "SELECT COUNT(1) AS count FROM " + aTableName
  );
  try {
    do_check_true(countStmt.executeStep());
    currentRows = countStmt.row.count;
  }
  finally {
    countStmt.finalize();
  }
  return currentRows;
}

cleanup();

