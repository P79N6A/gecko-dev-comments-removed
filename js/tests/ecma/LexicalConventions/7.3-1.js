















































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

var x = "// test \n testcase.actual = 'pass'"

testcase.actual = eval(x);

test();


function test() {
  for ( tc=0; tc < testcases.length; tc++ ) {
    testcases[tc].passed = writeTestCaseResult(
      testcases[tc].expect,
      testcases[tc].actual,
      testcases[tc].description +":  "+
      testcases[tc].actual );

    testcases[tc].reason += ( testcases[tc].passed ) ? "" : " ignored chars after line terminator of single-line comment";
  }
  stopTest();
  return ( testcases );
}
