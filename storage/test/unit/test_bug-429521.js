



function setup() 
{
    getOpenedDatabase().createTable("t1", "x TEXT");

    var stmt = createStatement("INSERT INTO t1 (x) VALUES ('/mozilla.org/20070129_1/Europe/Berlin')");
    stmt.execute();
    stmt.finalize();
}

function test_bug429521() 
{
    var stmt = createStatement(
        "SELECT DISTINCT(zone) FROM ("+
            "SELECT x AS zone FROM t1 WHERE x LIKE '/mozilla.org%'" +
        ");");

    print("*** test_bug429521: started");

    try {
        while (stmt.executeStep()) {
            print("*** test_bug429521: step() Read wrapper.row.zone");

            
            
            var tzId = stmt.row.zone;

            print("*** test_bug429521: step() Read wrapper.row.zone finished");
        }
    } catch (e) {
        print("*** test_bug429521: " + e);
    }

    print("*** test_bug429521: finished");

    stmt.finalize();
}

function run_test()
{
  setup();

  test_bug429521();
    
  cleanup();
}
