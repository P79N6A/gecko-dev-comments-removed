





































gTestfile = 'boolean-001.js';










var SECTION = "boolean-001.js";
var VERSION = "JS1_2";
startTest();
var TITLE   = "new Boolean(false) should evaluate to false";

writeHeaderToLog( SECTION + " "+ TITLE);

BooleanTest( "new Boolean(true)",  new Boolean(true),  true );
BooleanTest( "new Boolean(false)", new Boolean(false), false );
BooleanTest( "true",               true,               true );
BooleanTest( "false",              false,              false );

test();

function BooleanTest( string, object, expect ) {
  if ( object ) {
    result = true;
  } else {
    result = false;
  }

  new TestCase(
    SECTION,
    string,
    expect,
    result );
}

