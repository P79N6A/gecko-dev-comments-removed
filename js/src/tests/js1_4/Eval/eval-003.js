





































gTestfile = 'eval-003.js';






























var SECTION = "eval-003.js";
var VERSION = "JS1_4";
var TITLE   = "Calling eval indirectly should NOT fail in version 140";
var BUGNUMBER="38512";

startTest();
writeHeaderToLog( SECTION + " "+ TITLE);

var MY_EVAL = eval;
var RESULT = "";
var EXPECT= "";
var h = function f(x,y){var g = function(z){return Math.exp(z);}; return g(x+y);};
   

new EvalTest();

test();

function EvalTest()
{
  with( this ) {
    MY_EVAL( "RESULT = h(-1, 1)" );
    EXPECT = 1;  

    new TestCase(
      SECTION,
      "Call eval indirectly",
      EXPECT,
      RESULT );
  }
}

