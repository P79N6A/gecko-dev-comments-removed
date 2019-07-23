





































gTestfile = '15.8.2.14.js';















var SECTION = "15.8.2.14";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Math.random()";

writeHeaderToLog( SECTION + " "+ TITLE);

for ( var item = 0; item < 100; item++ ) {
  var testcase = new TestCase( SECTION, 
			       "Math.random()",   
			       "pass",   
			       null );
  testcase.reason = Math.random();
  testcase.actual = "pass";

  if ( ! ( testcase.reason >= 0) ) {
    testcase.actual = "fail";
  }

  if ( ! (testcase.reason < 1) ) {
    testcase.actual = "fail";
  }
}

test();
