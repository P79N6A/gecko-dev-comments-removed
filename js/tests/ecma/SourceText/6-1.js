





































gTestfile = '6-1.js';










































var SECTION = "6-1";
var VERSION = "ECMA_1";
startTest();
var TITLE   = "Source Text";

writeHeaderToLog( SECTION + " "+ TITLE);

var testcase = new TestCase( SECTION,
			     "// the following character should not be interpreted as a line terminator in a comment: \u000A",
			     'PASSED',
			     "PASSED" );



testcase =
  new TestCase( SECTION,
		"// the following character should not be interpreted as a line terminator in a comment: \\n 'FAILED'",
		'PASSED',
		'PASSED' );



testcase =
  new TestCase( SECTION,
		"// the following character should not be interpreted as a line terminator in a comment: \\u000A 'FAILED'",
		'PASSED',
		'PASSED' );



testcase =
  new TestCase( SECTION,
		"// the following character should not be interpreted as a line terminator in a comment: \n 'PASSED'",
		'PASSED',
		'PASSED' );


testcase =
  new TestCase(   SECTION,
		  "// the following character should not be interpreted as a line terminator in a comment: u000D",
		  'PASSED',
		  'PASSED' );



test();

