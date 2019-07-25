
















var SECTION = "try-006";
var VERSION = "ECMA_2";
var TITLE   = "The try statement";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);





function throwException() {
  throw EXCEPTION_STRING +": " + this.valueOf();
}
var EXCEPTION_STRING = "Exception thrown:";





function noException() {
  return this.valueOf();
}




TryWith( new TryObject( "hello", throwException, true ));
TryWith( new TryObject( "hola",  noException, false ));





test();




function TryObject( value, fun, exception ) {
  this.value = value;
  this.exception = exception;

  this.valueOf = new Function ( "return this.value" );
  this.check = fun;
}









function TryWith( object ) {
  try {
    with ( object ) {
      result = check();
    }
  } catch ( e ) {
    result = e;
  }

  new TestCase(
    SECTION,
    "TryWith( " + object.value +" )",
    (object.exception ? EXCEPTION_STRING +": " + object.valueOf() : object.valueOf()),
    result );
}
