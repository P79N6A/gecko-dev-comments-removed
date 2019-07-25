













var SECTION = "StrictEquality-001 - 11.9.6";
var VERSION = "ECMA_2";
var TITLE   =  "The strict equality operator ( === )";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);




StrictEquality( true, new Boolean(true), false );
StrictEquality( new Boolean(), false, false );
StrictEquality( "", new String(),    false );
StrictEquality( new String("hi"), "hi", false );




StrictEquality( NaN, NaN,   false );
StrictEquality( NaN, 0,     false );


StrictEquality( 0,  NaN,    false );
























test();

function StrictEquality( x, y, expect ) {
  result = ( x === y );

  new TestCase(
    SECTION,
    x +" === " + y,
    expect,
    result );
}

