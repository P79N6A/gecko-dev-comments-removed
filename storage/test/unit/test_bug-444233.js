







































var wrapper = new Components.Constructor("@mozilla.org/storage/statement-wrapper;1",
                                         Ci.mozIStorageStatementWrapper,
                                         "initialize");

createStatement = function(aSQL) {
    return new wrapper(getOpenedDatabase().createStatement(aSQL));
}

function setup() {
    
    getOpenedDatabase().createTable("test_bug444233",
                                    "id INTEGER PRIMARY KEY, value TEXT");

    
    var stmt = createStatement("INSERT INTO test_bug444233 (value) VALUES (:value)");
    stmt.params.value = "value1"
    stmt.execute();
    stmt.statement.finalize();
    
    stmt = createStatement("INSERT INTO test_bug444233 (value) VALUES (:value)");
    stmt.params.value = "value2"
    stmt.execute();
    stmt.statement.finalize();
}

function test_bug444233() {
    print("*** test_bug444233: started");
    
    
    var stmt = createStatement("SELECT COUNT(*) AS number FROM test_bug444233");
    do_check_true(stmt.step());
    do_check_eq(2, stmt.row.number);
    stmt.reset();
    stmt.statement.finalize();

    print("*** test_bug444233: doing delete");
    
    
    
    try {
        var ids = [1, 2];
        stmt = createStatement("DELETE FROM test_bug444233 WHERE id IN (:ids)");
        stmt.params.ids = ids;
    } catch (e) {
        print("*** test_bug444233: successfully caught exception");
    }
    stmt.statement.finalize();
}

function run_test() {
    setup();
    test_bug444233();
    cleanup();
}

