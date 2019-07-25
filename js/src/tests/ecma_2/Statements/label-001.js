
















var SECTION = "label-003";
var VERSION = "ECMA_2";
var TITLE   = "Labeled statements";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

LabelTest(0, 0);
LabelTest(1, 1)
  LabelTest(-1, 1000);
LabelTest(false,  0);
LabelTest(true, 1);

test();

function LabelTest( limit, expect) {
woo: for ( var result = 0; result < 1000; result++ ) { if (result == limit) { break woo; } else { continue woo; } };

  new TestCase(
    SECTION,
    "break out of a labeled for loop: "+ limit,
    expect,
    result );
}

