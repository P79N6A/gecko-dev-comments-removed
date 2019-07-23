







































































var SECTION = "10.1.4-1";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Scope Chain and Identifier Resolution");

new TestCase( "SECTION", 
	      "with MyObject, eval should cube INPUT:  " );
test();

function test() {
  for ( tc=0; tc < testcases.length; tc++ ) {

    var MYOBJECT = new MyObject();
    var INPUT = 2;
    testcases[tc].description += ( INPUT +"" );

    with ( MYOBJECT ) {
      eval = new Function ( "x", "return(Math.pow(Number(x),3))" );

      testcases[tc].actual = eval( INPUT );
      testcases[tc].expect = Math.pow(INPUT,3);
    }

    testcases[tc].passed = writeTestCaseResult(
      testcases[tc].expect,
      testcases[tc].actual,
      testcases[tc].description +" = "+
      testcases[tc].actual );

    testcases[tc].reason += ( testcases[tc].passed ) ? "" : "wrong value ";
  }
  stopTest();
  return ( testcases );
}

function MyObject() {
  this.eval = new Function( "x", "return(Math.pow(Number(x),2))" );
}
