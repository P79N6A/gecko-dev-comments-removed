

















var SECTION = "";
var VERSION = "ECMA_2";
var TITLE   = "The try statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var INVALID_JAVA_INTEGER_VALUE = "Invalid value for java.lang.Integer constructor";

TryNewJavaInteger( "3.14159", INVALID_JAVA_INTEGER_VALUE );
TryNewJavaInteger( NaN, INVALID_JAVA_INTEGER_VALUE );
TryNewJavaInteger( 0,  0 );
TryNewJavaInteger( -1, -1 );
TryNewJavaInteger( 1,  1 );
TryNewJavaInteger( Infinity, Infinity );

test();








function newJavaInteger( v ) {
  value = Number( v );
  if ( Math.floor(value) != value || isNaN(value) ) {
    throw ( INVALID_JAVA_INTEGER_VALUE );
  } else {
    return value;
  }
}








function TryNewJavaInteger( value, expect ) {
  var finalTest = false;

  try {
    result = newJavaInteger( value );
  } catch ( e ) {
    result = String( e );
  } finally {
    finalTest = true;
  }
  new TestCase(
    SECTION,
    "newJavaValue( " + value +" )",
    expect,
    result);

  new TestCase(
    SECTION,
    "newJavaValue( " + value +" ) hit finally block",
    true,
    finalTest);

}

