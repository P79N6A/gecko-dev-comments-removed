





var SECTION = "exception-011";
var VERSION = "ECMA_2";
startTest();
var TITLE   = "Don't Crash throwing undefined";

writeHeaderToLog( SECTION + " "+ TITLE);

print("Undefined throw test.");

DESCRIPTION = "throw undefined";
EXPECTED = "error";

new TestCase( SECTION,  "throw undefined",  "error", eval("throw (void 0)") );

test();

print("FAILED!: Should have exited with uncaught exception.");



