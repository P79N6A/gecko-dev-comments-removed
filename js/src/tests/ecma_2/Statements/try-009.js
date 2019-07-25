

















var SECTION = "try-009";
var VERSION = "ECMA_2";
var TITLE   = "The try statement: try in a while block";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var EXCEPTION_STRING = "Exception thrown: ";
var NO_EXCEPTION_STRING = "No exception thrown: ";


TryInWhile( new TryObject( "hello", ThrowException, true ) );
TryInWhile( new TryObject( "aloha", NoException, false ));

test();

function TryObject( value, throwFunction, result ) {
  this.value = value;
  this.thrower = throwFunction;
  this.result = result;
}
function ThrowException() {
  throw EXCEPTION_STRING + this.value;
}
function NoException() {
  return NO_EXCEPTION_STRING + this.value;
}
function TryInWhile( object ) {
  result = null;
  while ( true ) {
    try {
      object.thrower();
      result = NO_EXCEPTION_STRING + object.value;
      break;
    } catch ( e ) {
      result = e;
      break;
    }
  }

  new TestCase(
    SECTION,
    "( "+ object  +".thrower() )",
    (object.result
     ? EXCEPTION_STRING + object.value :
     NO_EXCEPTION_STRING + object.value),
    result );
}
