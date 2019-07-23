






































function createStatementWrapper(aSQL) 
{
    var stmt = getOpenedDatabase().createStatement(aSQL);
    var wrapper = Components.Constructor("@mozilla.org/storage/statement-wrapper;1", Ci.mozIStorageStatementWrapper)();
    wrapper.initialize(stmt);
    return wrapper;
}

function setup() 
{
    getOpenedDatabase().createTable("t1", "x TEXT");

    var stmt = getOpenedDatabase().createStatement("INSERT INTO t1 (x) VALUES ('/mozilla.org/20070129_1/Europe/Berlin')");
    stmt.execute();
    stmt.finalize();
}

function test_bug429521() 
{
    var wrapper = createStatementWrapper(
        "SELECT DISTINCT(zone) FROM ("+
            "SELECT x AS zone FROM t1 WHERE x LIKE '/mozilla.org%'" +
        ");");

    print("*** test_bug429521: started");

    try {
        while (wrapper.step()) {
            print("*** test_bug429521: step() Read wrapper.row.zone");

            
            
            var tzId = wrapper.row.zone;

            print("*** test_bug429521: step() Read wrapper.row.zone finished");
        }
    } catch (e) {
        print("*** test_bug429521: " + e);
    }

    print("*** test_bug429521: finished");

    wrapper.statement.finalize();
}

function run_test()
{
  setup();

  test_bug429521();
    
  cleanup();
}
