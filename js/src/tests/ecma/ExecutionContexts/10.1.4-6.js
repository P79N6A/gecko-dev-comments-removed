





































gTestfile = '10.1.4-6.js';



































var SECTION = "10.1.4-6";
var VERSION = "ECMA_1";
startTest();

writeHeaderToLog( SECTION + " Scope Chain and Identifier Resolution");


var testcase = new TestCase( "SECTION",
			     "with MyObject, eval should be [object Global].eval " );

var MYOBJECT = new MyObject();
var INPUT = 2;
testcase.description += ( INPUT +"" );

with ( MYOBJECT ) {
  ;
}
testcase.actual = eval( INPUT );
testcase.expect = INPUT;

test();


function MyObject() {
  this.eval = new Function( "x", "return(Math.pow(Number(x),2))" );
}
