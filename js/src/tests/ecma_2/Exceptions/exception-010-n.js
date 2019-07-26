





var SECTION = "exception-010";
var VERSION = "ECMA_2";
startTest();
var TITLE   = "Don't Crash throwing null";

writeHeaderToLog( SECTION + " "+ TITLE);
print("Null throw test.");
print("BUGNUMBER: 21799");

DESCRIPTION = "throw null";
EXPECTED = "error";

new TestCase( SECTION,  "throw null",     "error",    eval("throw null" ));

test();

print("FAILED!: Should have exited with uncaught exception.");


