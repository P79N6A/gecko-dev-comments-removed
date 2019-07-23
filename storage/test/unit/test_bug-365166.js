
 




function run_test() {
  test('user');
  test('schema');

  function test(param)
  {
    var colName = param + "_version";
    var sql = "PRAGMA " + colName;

    var file = Components.classes["@mozilla.org/file/directory_service;1"]
                         .getService(Components.interfaces.nsIProperties)
                         .get("TmpD", Components.interfaces.nsIFile);
    file.append("bug-365166.sqlite");
    var storageService = Components.classes["@mozilla.org/storage/service;1"].
                         getService(Components.interfaces.mozIStorageService);
    var conn = storageService.openDatabase(file); 
    var statement = conn.createStatement(sql);
    try {
      
      do_check_eq(statement.getColumnName(0), colName);

      
      
      var wrapper = Components.classes["@mozilla.org/storage/statement-wrapper;1"]
                              .createInstance(Components.interfaces.mozIStorageStatementWrapper);
      wrapper.initialize(statement);
    } finally {
      statement.reset();
    }
  }
}
