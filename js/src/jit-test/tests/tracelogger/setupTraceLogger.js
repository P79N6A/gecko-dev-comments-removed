
var du = new Debugger();
if (typeof du.setupTraceLogger == "function") {

    
    assertEq(du.setupTraceLogger({
        Scripts: true
    }), true);

    
    assertEq(du.setupTraceLogger({
        Scripts: true
    }), true);

    
    assertEq(du.setupTraceLogger({
        Scripts: false
    }), true);

    
    assertEq(du.setupTraceLogger({
        Scripts: false
    }), true);

    
    var success = du.setupTraceLogger({
        Scripts: false,
        Test: true
    });
    assertEq(success, false);

    
    
    du.startTraceLogger();
    var obj = du.drainTraceLogger();
    du.setupTraceLogger({
        Scripts: true,
        Test: true,
    });
    assertEq(du.drainTraceLogger().length, 0);
    du.endTraceLogger();

    
    succes = du.setupTraceLogger("blaat");
    assertEq(succes, false);

    
    succes = du.setupTraceLogger("blaat");
    assertEq(succes, false);

    
    failed = false;
    try {
        du.setupTraceLogger();
    } catch (e) {
        failed = true;
    }
    assertEq(failed, true);

    
    succes = du.setupTraceLogger({}, "test");
    assertEq(succes, true);
}

var du2 = new Debugger();
if (typeof du2.setupTraceLoggerForTraces == "function") {
    du2.setupTraceLoggerForTraces({});
    du2.setupTraceLoggerForTraces("test");
    du2.setupTraceLoggerForTraces({}, "test");
}
