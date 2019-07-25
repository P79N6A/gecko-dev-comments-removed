

















var SECTION = "try-010";
var VERSION = "ECMA_2";
var TITLE   = "The try statement: try in a tryblock";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var EXCEPTION_STRING = "Exception thrown: ";
var NO_EXCEPTION_STRING = "No exception thrown:  ";


NestedTry( new TryObject( "No Exceptions Thrown",  NoException, NoException, 43 ) );
NestedTry( new TryObject( "Throw Exception in Outer Try", ThrowException, NoException, 48 ));
NestedTry( new TryObject( "Throw Exception in Inner Try", NoException, ThrowException, 45 ));
NestedTry( new TryObject( "Throw Exception in Both Trys", ThrowException, ThrowException, 48 ));

test();

function TryObject( description, tryOne, tryTwo, result ) {
  this.description = description;
  this.tryOne = tryOne;
  this.tryTwo = tryTwo;
  this.result = result;
}
function ThrowException() {
  throw EXCEPTION_STRING + this.value;
}
function NoException() {
  return NO_EXCEPTION_STRING + this.value;
}
function NestedTry( object ) {
  result = 0;
  try {
    object.tryOne();
    result += 1;
    try {
      object.tryTwo();
      result += 2;
    } catch ( e ) {
      result +=4;
    } finally {
      result += 8;
    }
  } catch ( e ) {
    result += 16;
  } finally {
    result += 32;
  }

  new TestCase(
    SECTION,
    object.description,
    object.result,
    result );
}
