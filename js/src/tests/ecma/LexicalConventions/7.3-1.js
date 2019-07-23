





































gTestfile = '7.3-1.js';











var SECTION = "7.3-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Comments";

writeHeaderToLog( SECTION + " "+ TITLE);

var testcase;

testcase = new TestCase( SECTION,
			 "a comment with a line terminator string, and text following",
			 "pass",
			 "pass");




testcase = new TestCase( SECTION,
			 "// test \\n testcase.actual = \"pass\"",
			 "pass",
			 "" );

var x = "// test \n testcase.actual = 'pass'";

testcase.actual = eval(x);

test();


function test() {
  for ( gTc=0; gTc < gTestcases.length; gTc++ ) {
    gTestcases[gTc].passed = writeTestCaseResult(
      gTestcases[gTc].expect,
      gTestcases[gTc].actual,
      gTestcases[gTc].description +":  "+
      gTestcases[gTc].actual );

    gTestcases[gTc].reason += ( gTestcases[gTc].passed ) ? "" : " ignored chars after line terminator of single-line comment";
  }
  stopTest();
  return ( gTestcases );
}
