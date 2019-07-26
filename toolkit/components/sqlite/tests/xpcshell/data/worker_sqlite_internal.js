


importScripts("worker_sqlite_shared.js",
  "resource://gre/modules/workers/require.js");

self.onmessage = function onmessage(msg) {
  try {
    run_test();
  } catch (ex) {
    let {message, moduleStack, moduleName, lineNumber} = ex;
    let error = new Error(message, moduleName, lineNumber);
    error.stack = moduleStack;
    dump("Uncaught error: " + error + "\n");
    dump("Full stack: " + moduleStack + "\n");
    throw error;
  }
};

let Sqlite;

let SQLITE_OK;   
let SQLITE_ROW;  
let SQLITE_DONE; 

function test_init() {
  do_print("Starting test_init");
  
  Sqlite = require("resource://gre/modules/sqlite/sqlite_internal.js");
  do_check_neq(typeof Sqlite, "undefined");
  do_check_neq(typeof Sqlite.Constants, "undefined");
  SQLITE_OK = Sqlite.Constants.SQLITE_OK;
  SQLITE_ROW = Sqlite.Constants.SQLITE_ROW;
  SQLITE_DONE = Sqlite.Constants.SQLITE_DONE;
}





function cleanupDB(db) {
  withQuery(db, "DROP TABLE IF EXISTS TEST;", SQLITE_DONE);
}










function withDB(open, openArgs = [], callback = null) {
  let db = Sqlite.Type.sqlite3_ptr.implementation();
  let dbPtr = db.address();

  
  let result = Sqlite[open].apply(Sqlite, ["data/test.db", dbPtr].concat(
    openArgs));
  do_check_eq(result, SQLITE_OK);

  
  cleanupDB(db);

  try {
    if (callback) {
      callback(db);
    }
  } catch (ex) {
    do_check_true(false);
    throw ex;
  } finally {
    
    cleanupDB(db);
    
    result = Sqlite.close(db);
    do_check_eq(result, SQLITE_OK);
  }
}











function withQuery(db, sql, stepResult, bind, callback) {
  
  let sqlStmt = Sqlite.Type.sqlite3_stmt_ptr.implementation();
  let sqlStmtPtr = sqlStmt.address();

  
  let unused = Sqlite.Type.cstring.implementation();
  let unusedPtr = unused.address();

  
  let result = Sqlite.prepare_v2(db, sql, sql.length, sqlStmtPtr, unusedPtr);
  do_check_eq(result, SQLITE_OK);

  try {
    if (bind) {
      bind(sqlStmt);
    }

    
    result = Sqlite.step(sqlStmt);
    do_check_eq(result, stepResult);

    if (callback) {
      callback(sqlStmt);
    }
  } catch (ex) {
    do_check_true(false);
    throw ex;
  } finally {
    
    result = Sqlite.finalize(sqlStmt);
    do_check_eq(result, SQLITE_OK);
  }
}

function test_open_close() {
  do_print("Starting test_open_close");
  do_check_eq(typeof Sqlite.open, "function");
  do_check_eq(typeof Sqlite.close, "function");

  withDB("open");
}

function test_open_v2_close() {
  do_print("Starting test_open_v2_close");
  do_check_eq(typeof Sqlite.open_v2, "function");

  withDB("open_v2", [0x02, null]);
}

function createTableOnOpen(db) {
  withQuery(db, "CREATE TABLE TEST(" +
              "ID INT PRIMARY KEY NOT NULL," +
              "FIELD1 INT," +
              "FIELD2 REAL," +
              "FIELD3 TEXT," +
              "FIELD4 TEXT," +
              "FIELD5 BLOB" +
            ");", SQLITE_DONE);
}

function test_create_table() {
  do_print("Starting test_create_table");
  do_check_eq(typeof Sqlite.prepare_v2, "function");
  do_check_eq(typeof Sqlite.step, "function");
  do_check_eq(typeof Sqlite.finalize, "function");

  withDB("open", [], createTableOnOpen);
}





function onSqlite3Step(sqlStmt) {
  
  let field = Sqlite.column_int(sqlStmt, 0);
  do_check_eq(field, 3);

  
  field = Sqlite.column_int(sqlStmt, 1);
  do_check_eq(field, 2);
  
  field = Sqlite.column_int64(sqlStmt, 1);
  do_check_eq(field, 2);

  
  field = Sqlite.column_double(sqlStmt, 2);
  do_check_eq(field, 1.2);

  
  let bytes = Sqlite.column_bytes(sqlStmt, 3);
  do_check_eq(bytes, 4);
  
  field = Sqlite.column_text(sqlStmt, 3);
  do_check_eq(field.readString(), "DATA");

  
  bytes = Sqlite.column_bytes16(sqlStmt, 4);
  do_check_eq(bytes, 8);
  
  field = Sqlite.column_text16(sqlStmt, 4);
  do_check_eq(field.readString(), "TADA");

  
  field = Sqlite.column_blob(sqlStmt, 5);
  do_check_eq(ctypes.cast(field,
    Sqlite.Type.cstring.implementation).readString(), "BLOB");
}

function test_insert_select() {
  do_print("Starting test_insert_select");
  do_check_eq(typeof Sqlite.column_int, "function");
  do_check_eq(typeof Sqlite.column_int64, "function");
  do_check_eq(typeof Sqlite.column_double, "function");
  do_check_eq(typeof Sqlite.column_bytes, "function");
  do_check_eq(typeof Sqlite.column_text, "function");
  do_check_eq(typeof Sqlite.column_text16, "function");
  do_check_eq(typeof Sqlite.column_blob, "function");

  function onOpen(db) {
    createTableOnOpen(db);
    withQuery(db,
      "INSERT INTO TEST VALUES (3, 2, 1.2, \"DATA\", \"TADA\", \"BLOB\");",
      SQLITE_DONE);
    withQuery(db, "SELECT * FROM TEST;", SQLITE_ROW, null, onSqlite3Step);
  }

  withDB("open", [], onOpen);
}

function test_insert_bind_select() {
  do_print("Starting test_insert_bind_select");
  do_check_eq(typeof Sqlite.bind_int, "function");
  do_check_eq(typeof Sqlite.bind_int64, "function");
  do_check_eq(typeof Sqlite.bind_double, "function");
  do_check_eq(typeof Sqlite.bind_text, "function");
  do_check_eq(typeof Sqlite.bind_text16, "function");
  do_check_eq(typeof Sqlite.bind_blob, "function");

  function onBind(sqlStmt) {
    
    let result = Sqlite.bind_int(sqlStmt, 1, 3);
    do_check_eq(result, SQLITE_OK);

    
    result = Sqlite.bind_int64(sqlStmt, 2, 2);
    do_check_eq(result, SQLITE_OK);

    
    result = Sqlite.bind_double(sqlStmt, 3, 1.2);
    do_check_eq(result, SQLITE_OK);

    
    let destructor = Sqlite.Constants.SQLITE_TRANSIENT;
    
    result = Sqlite.bind_text(sqlStmt, 4, "DATA", 4, destructor);
    do_check_eq(result, SQLITE_OK);

    
    result = Sqlite.bind_text16(sqlStmt, 5, "TADA", 8, destructor);
    do_check_eq(result, SQLITE_OK);

    
    result = Sqlite.bind_blob(sqlStmt, 6, ctypes.char.array()("BLOB"), 4,
      destructor);
    do_check_eq(result, SQLITE_OK);
  }

  function onOpen(db) {
    createTableOnOpen(db);
    withQuery(db, "INSERT INTO TEST VALUES (?, ?, ?, ?, ?, ?);", SQLITE_DONE,
      onBind);
    withQuery(db, "SELECT * FROM TEST;", SQLITE_ROW, null, onSqlite3Step);
  }

  withDB("open", [], onOpen);
}

function run_test() {
  test_init();
  test_open_close();
  test_open_v2_close();
  test_create_table();
  test_insert_select();
  test_insert_bind_select();
  do_test_complete();
}
