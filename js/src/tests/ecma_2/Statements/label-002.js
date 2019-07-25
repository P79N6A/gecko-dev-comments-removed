
















var SECTION = "label-002";
var VERSION = "ECMA_2";
var TITLE   = "Labeled statements";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

LabelTest( { p1:"hi,", p2:" norris" }, "hi, norris", " norrishi," );
LabelTest( { 0:"zero", 1:"one" }, "zeroone", "onezero" );

LabelTest2( { p1:"hi,", p2:" norris" }, "hi,", " norris" );
LabelTest2( { 0:"zero", 1:"one" }, "zero", "one" );

test();

function LabelTest( object, expect1, expect2 ) {
  result = "";

yoohoo:  { for ( property in object ) { result += object[property]; }; break yoohoo };

  new TestCase(
    SECTION,
    "yoohoo: for ( property in object ) { result += object[property]; } break yoohoo }",
    true,
    result == expect1 || result == expect2 );
}

function LabelTest2( object, expect1, expect2 ) {
  result = "";

yoohoo:  { for ( property in object ) { result += object[property]; break yoohoo } }; ;

  new TestCase(
    SECTION,
    "yoohoo: for ( property in object ) { result += object[property]; break yoohoo }}",
    true,
    result == expect1 || result == expect2 );
}

