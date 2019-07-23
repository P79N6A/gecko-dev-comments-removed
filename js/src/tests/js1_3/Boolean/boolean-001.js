





































gTestfile = 'boolean-001.js';










var SECTION = "boolean-001.js";
var VERSION = "JS_1.3";
var TITLE   = "new Boolean(false) should evaluate to false";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

BooleanTest( "new Boolean(true)",  new Boolean(true),  true );
BooleanTest( "new Boolean(false)", new Boolean(false), true );
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

