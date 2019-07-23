






































const BACKUP_FILE_NAME = "test_storage.sqlite.backup";

function test_connectionReady_open()
{
  
  
  
  

  var msc = getOpenedDatabase();
  do_check_true(msc.connectionReady);
}

function test_connectionReady_closed()
{
  

  var msc = getOpenedDatabase();
  msc.close();
  do_check_false(msc.connectionReady);
  gDBConn = null; 
}

function test_databaseFile()
{
  var msc = getOpenedDatabase();
  do_check_true(getTestDB().equals(msc.databaseFile));
}

function test_tableExists_not_created()
{
  var msc = getOpenedDatabase();
  do_check_false(msc.tableExists("foo"));
}

function test_indexExists_not_created()
{
  var msc = getOpenedDatabase();
  do_check_false(msc.indexExists("foo"));
}

function test_createTable_not_created()
{
  var msc = getOpenedDatabase();
  msc.createTable("test", "id INTEGER PRIMARY KEY, name TEXT");
  do_check_true(msc.tableExists("test"));
}

function test_indexExists_created()
{
  var msc = getOpenedDatabase();
  msc.executeSimpleSQL("CREATE INDEX name_ind ON test (name)");
  do_check_true(msc.indexExists("name_ind"));
}

function test_createTable_already_created()
{
  var msc = getOpenedDatabase();
  do_check_true(msc.tableExists("test"));
  try {
    msc.createTable("test", "id INTEGER PRIMARY KEY, name TEXT");
    do_throw("We shouldn't get here!");
  } catch (e) {
    do_check_eq(Cr.NS_ERROR_FAILURE, e.result);
  }
}

function test_lastInsertRowID()
{
  var msc = getOpenedDatabase();
  msc.executeSimpleSQL("INSERT INTO test (name) VALUES ('foo')");
  do_check_eq(1, msc.lastInsertRowID);
}

function test_transactionInProgress_no()
{
  var msc = getOpenedDatabase();
  do_check_false(msc.transactionInProgress);
}

function test_transactionInProgress_yes()
{
  var msc = getOpenedDatabase();
  msc.beginTransaction();
  do_check_true(msc.transactionInProgress);
  msc.commitTransaction();
  do_check_false(msc.transactionInProgress);

  msc.beginTransaction();
  do_check_true(msc.transactionInProgress);
  msc.rollbackTransaction();
  do_check_false(msc.transactionInProgress);
}

function test_commitTransaction_no_transaction()
{
  var msc = getOpenedDatabase();
  do_check_false(msc.transactionInProgress);
  try {
    msc.commitTransaction();
    do_throw("We should not get here!");
  } catch (e) {
    do_check_eq(Cr.NS_ERROR_FAILURE, e.result);
  }
}

function test_rollbackTransaction_no_transaction()
{
  var msc = getOpenedDatabase();
  do_check_false(msc.transactionInProgress);
  try {
    msc.rollbackTransaction();
    do_throw("We should not get here!");
  } catch (e) {
    do_check_eq(Cr.NS_ERROR_FAILURE, e.result);
  }
}

function test_get_schemaVersion_not_set()
{
  do_check_eq(0, getOpenedDatabase().schemaVersion);
}

function test_set_schemaVersion()
{
  var msc = getOpenedDatabase();
  const version = 1;
  msc.schemaVersion = version;
  do_check_eq(version, msc.schemaVersion);
}

function test_set_schemaVersion_same()
{
  var msc = getOpenedDatabase();
  const version = 1;
  msc.schemaVersion = version; 
  do_check_eq(version, msc.schemaVersion);
}

function test_set_schemaVersion_negative()
{
  var msc = getOpenedDatabase();
  const version = -1;
  msc.schemaVersion = version;
  do_check_eq(version, msc.schemaVersion);
}

function test_backup_not_new_filename()
{
  var msc = getOpenedDatabase();
  const fname = getTestDB().leafName;

  var backup = msc.backupDB(fname);
  do_check_neq(fname, backup.leafName);

  backup.remove(false);
}

function test_backup_new_filename()
{
  var msc = getOpenedDatabase();

  var backup = msc.backupDB(BACKUP_FILE_NAME);
  do_check_eq(BACKUP_FILE_NAME, backup.leafName);
  
  backup.remove(false);
}

function test_backup_new_folder()
{
  var msc = getOpenedDatabase();
  var parentDir = getTestDB().parent;
  parentDir.append("test_storage_temp");
  if (parentDir.exists())
    parentDir.remove(true);
  parentDir.create(Ci.nsIFile.DIRECTORY_TYPE, 0755);
  do_check_true(parentDir.exists());

  var backup = msc.backupDB(BACKUP_FILE_NAME, parentDir);
  do_check_eq(BACKUP_FILE_NAME, backup.leafName);
  do_check_true(parentDir.equals(backup.parent));

  parentDir.remove(true);
}

function test_createTable(){
  var temp = getTestDB().parent;
  temp.append("test_db_table");
  try {
    var con = getService().openDatabase(temp);
    con.createTable("a","");
  } catch (e) {
    if (temp.exists()) try {
      temp.remove(false);
    } catch (e2) {}
    do_check_true(e.result==Cr.NS_ERROR_NOT_INITIALIZED ||
                  e.result==Cr.NS_ERROR_FAILURE);
  }
}

var tests = [
  test_connectionReady_open,
  test_connectionReady_closed,
  test_databaseFile,
  test_tableExists_not_created,
  test_indexExists_not_created,
  test_createTable_not_created,
  test_indexExists_created,
  test_createTable_already_created,
  test_lastInsertRowID,
  test_transactionInProgress_no,
  test_transactionInProgress_yes,
  test_commitTransaction_no_transaction,
  test_rollbackTransaction_no_transaction,
  test_get_schemaVersion_not_set,
  test_set_schemaVersion,
  test_set_schemaVersion_same,
  test_set_schemaVersion_negative,
  test_backup_not_new_filename,
  test_backup_new_filename,
  test_backup_new_folder,
  test_createTable,
];

function run_test()
{
  for (var i = 0; i < tests.length; i++)
    tests[i]();
    
  cleanup();
}
