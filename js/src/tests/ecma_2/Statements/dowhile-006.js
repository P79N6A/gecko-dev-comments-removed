















var SECTION = "dowhile-006";
var VERSION = "ECMA_2";
var TITLE   = "do...while";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

DoWhile( new DoWhileObject( false, false, 10 ) );
DoWhile( new DoWhileObject( true, false, 2 ) );
DoWhile( new DoWhileObject( false, true, 3 ) );
DoWhile( new DoWhileObject( true, true, 4 ) );

test();

function looping( object ) {
  object.iterations--;

  if ( object.iterations <= 0 ) {
    return false;
  } else {
    return true;
  }
}
function DoWhileObject( breakOut, breakIn, iterations, loops ) {
  this.iterations = iterations;
  this.loops = loops;
  this.breakOut = breakOut;
  this.breakIn  = breakIn;
  this.looping  = looping;
}
function DoWhile( object ) {
  var result1 = false;
  var result2 = false;

outie: {
  innie: {
      do {
	if ( object.breakOut )
	  break outie;

	if ( object.breakIn )
	  break innie;

      } while ( looping(object) );

      
      
      

      result1 = true;

    }






    result2 = true;
  }

  new TestCase(
    SECTION,
    "hit code after loop in inner loop",
    ( object.breakIn || object.breakOut ) ? false : true ,
    result1 );

  new TestCase(
    SECTION,
    "hit code after loop in outer loop",
    ( object.breakOut ) ? false : true,
    result2 );
}
