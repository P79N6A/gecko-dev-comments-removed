








































const CONN_LIST =
{
  S1: { shared: true },
  P1: { shared: false },
  S2: { shared: true },
  P2: { shared: false }
};




function TestBody()
{
  this.conn = new Array();
  this.dbFile = new Array();

  for (var curConn in CONN_LIST) {
    var db = dirSvc.get("ProfD", Ci.nsIFile);
    db.append("test_storage_" + curConn + ".sqlite");
    this.dbFile[curConn] = db;
  }

  this.cleanUp();
}




TestBody.prototype.cleanUp = function cleanUp()
{
  for (var curConn in CONN_LIST) {
    if (this.dbFile[curConn].exists()) {
      try {
        this.dbFile[curConn].remove(false);
      }
      catch(e) {
      
      }
    }
  }
}




TestBody.prototype.test_initialize_database = 
function test_initialize_database()
{
  for (var curConn in CONN_LIST) {
    if (CONN_LIST[curConn].shared)
      this.conn[curConn] = getService().openDatabase(this.dbFile[curConn]);
    else
      this.conn[curConn] = getService().openUnsharedDatabase(
                                          this.dbFile[curConn]);
    do_check_true(this.conn[curConn].connectionReady);
  }
}







TestBody.prototype.test_create_tables = 
function test_create_tables()
{
  var realSql = "CREATE TABLE book (author TEXT, title TEXT)";
  var virtSql = "CREATE VIRTUAL TABLE book USING fts3(author, title)";

  for (var curConn in CONN_LIST) {
    this.conn[curConn].createTable("test", "id INTEGER PRIMARY KEY, name TEXT");
    do_check_true(this.conn[curConn].tableExists("test"));

    try {
      this.conn[curConn].executeSimpleSQL(virtSql);
      if (CONN_LIST[curConn].shared) 
        do_throw("We shouldn't be able to create virtual tables on " +
                 curConn + " database!");
    }
    catch (e) {
      
      
      this.conn[curConn].executeSimpleSQL(realSql);
    }

    do_check_true(this.conn[curConn].tableExists("book"));
  }
}





TestBody.prototype.test_real_table_insert_select = 
function test_real_table_insert_select()
{
  var stmts = new Array();

  for (var curConn in CONN_LIST)
    this.conn[curConn].beginTransaction();

  for (var curConn in CONN_LIST)
    this.conn[curConn].executeSimpleSQL(
        "INSERT INTO test (name) VALUES ('Test')");

  for (var curConn in CONN_LIST)
    stmts[curConn] = this.conn[curConn].createStatement("SELECT * FROM test");

  for (var curConn in CONN_LIST)
    stmts[curConn].executeStep();

  for (var curConn in CONN_LIST)
    do_check_eq(1, stmts[curConn].getInt32(0));

  for (var curConn in CONN_LIST)
    stmts[curConn].reset();

  for (var curConn in CONN_LIST)
    stmts[curConn].finalize();

  for (var curConn in CONN_LIST)
    this.conn[curConn].commitTransaction();
}





TestBody.prototype.test_virtual_table_insert_select = 
function test_virtual_table_insert_select()
{
  var stmts = new Array();

  for (var curConn in CONN_LIST)
    this.conn[curConn].beginTransaction();

  for (var curConn in CONN_LIST)
    this.conn[curConn].executeSimpleSQL(
        "INSERT INTO book VALUES ('Frank Herbert', 'The Dune')");

  for (var curConn in CONN_LIST)
    stmts[curConn] = this.conn[curConn].createStatement(
                         "SELECT * FROM book WHERE author >= 'Frank'");

  for (var curConn in CONN_LIST)
    stmts[curConn].executeStep();

  for (var curConn in CONN_LIST)
    do_check_eq("Frank Herbert", stmts[curConn].getString(0));

  for (var curConn in CONN_LIST)
    stmts[curConn].reset();

  for (var curConn in CONN_LIST)
    stmts[curConn].finalize();

  for (var curConn in CONN_LIST)
    this.conn[curConn].commitTransaction();
}

var tests = [
  "test_initialize_database",
  "test_create_tables",
  "test_real_table_insert_select",
  "test_virtual_table_insert_select"
];

function run_test()
{
  var tb = new TestBody;

  try {
    for (var i = 0; i < tests.length; ++i)
      tb[tests[i]]();
  }
  finally {
    for (var curConn in CONN_LIST) {
      var errStr = tb.conn[curConn].lastErrorString;
      if (errStr != "not an error") 
        print("*** Database error: " + errStr);
      tb.conn[curConn].close();
    }
    tb.cleanUp(); 
  }
}
