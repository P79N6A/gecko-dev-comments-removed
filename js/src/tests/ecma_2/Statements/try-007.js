
















var SECTION = "try-007";
var VERSION = "ECMA_2";
var TITLE   = "The try statement:  for-in";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);





function throwException() {
  throw EXCEPTION_STRING +": " + this.valueOf();
}
var EXCEPTION_STRING = "Exception thrown:";





function noException() {
  return this.valueOf();
}




TryForIn( new TryObject( "hello", throwException, true ));
TryForIn( new TryObject( "hola",  noException, false ));





test();







function TryObject( value, fun, exception ) {
  this.value = value;
  this.exception = exception;

  this.check = fun;
  this.valueOf = function () { return this.value; }
}








function TryForIn( object ) {
  try {
    for ( p in object ) {
      if ( typeof object[p] == "function" ) {
	result = object[p]();
      }
    }
  } catch ( e ) {
    result = e;
  }

  new TestCase(
    SECTION,
    "TryForIn( " + object+ " )",
    (object.exception ? EXCEPTION_STRING +": " + object.value : object.value),
    result );

}
