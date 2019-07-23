





































gTestfile = '10.1.4-8.js';



































var SECTION = "10.1.4-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Scope Chain and Identifier Resolution");

new TestCase( "SECTION",
	      "with MyObject, eval should cube INPUT:  " );
test();

function test() {
  for ( gTc=0; gTc < gTestcases.length; gTc++ ) {

    var MYOBJECT = new MyObject();
    var INPUT = 2;
    gTestcases[gTc].description += ( INPUT +"" );

    with ( MYOBJECT ) {
      eval = new Function ( "x", "return(Math.pow(Number(x),3))" );

      gTestcases[gTc].actual = eval( INPUT );
      gTestcases[gTc].expect = Math.pow(INPUT,3);
    }

    gTestcases[gTc].passed = writeTestCaseResult(
      gTestcases[gTc].expect,
      gTestcases[gTc].actual,
      gTestcases[gTc].description +" = "+
      gTestcases[gTc].actual );

    gTestcases[gTc].reason += ( gTestcases[gTc].passed ) ? "" : "wrong value ";
  }
  stopTest();
  return ( gTestcases );
}

function MyObject() {
  this.eval = new Function( "x", "return(Math.pow(Number(x),2))" );
}
