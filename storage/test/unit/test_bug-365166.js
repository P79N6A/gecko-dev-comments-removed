
 




function run_test() {
  test('user');
  test('schema');

  function test(param)
  {
    var colName = param + "_version";
    var sql = "PRAGMA " + colName;

    var file = getTestDB();
    var storageService = Components.classes["@mozilla.org/storage/service;1"].
                         getService(Components.interfaces.mozIStorageService);
    var conn = storageService.openDatabase(file); 
    var statement = conn.createStatement(sql);
    try {
      
      do_check_eq(statement.getColumnName(0), colName);
    } finally {
      statement.reset();
      statement.finalize();
    }
  }
}
