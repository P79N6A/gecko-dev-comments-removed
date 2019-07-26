





print("STATUS: f.apply crash test.");

print("BUGNUMBER: 21836");

function f ()
{
}

var SECTION = "apply-001-n";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "f.apply(2,2) doesn't crash";

writeHeaderToLog( SECTION + " "+ TITLE);

DESCRIPTION = "f.apply(2,2) doesn't crash";
EXPECTED = "error";

new TestCase( SECTION,  "f.apply(2,2) doesn't crash",     "error",    eval("f.apply(2,2)") );

test();


