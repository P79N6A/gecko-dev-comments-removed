





































gTestfile = 'compile.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: compile';

writeHeaderToLog('Executing script: compile.js');
writeHeaderToLog( SECTION + " "+ TITLE);

var regularExpression = new RegExp();

regularExpression.compile("[0-9]{3}x[0-9]{4}","i");

new TestCase ( SECTION,
	       "(compile '[0-9]{3}x[0-9]{4}','i')",
	       String(["456X7890"]), String('234X456X7890'.match(regularExpression)));

new TestCase ( SECTION,
	       "source of (compile '[0-9]{3}x[0-9]{4}','i')",
	       "[0-9]{3}x[0-9]{4}", regularExpression.source);

new TestCase ( SECTION,
	       "global of (compile '[0-9]{3}x[0-9]{4}','i')",
	       false, regularExpression.global);

new TestCase ( SECTION,
	       "ignoreCase of (compile '[0-9]{3}x[0-9]{4}','i')",
	       true, regularExpression.ignoreCase);

regularExpression.compile("[0-9]{3}X[0-9]{3}","g");

new TestCase ( SECTION,
	       "(compile '[0-9]{3}X[0-9]{3}','g')",
	       String(["234X456"]), String('234X456X7890'.match(regularExpression)));

new TestCase ( SECTION,
	       "source of (compile '[0-9]{3}X[0-9]{3}','g')",
	       "[0-9]{3}X[0-9]{3}", regularExpression.source);

new TestCase ( SECTION,
	       "global of (compile '[0-9]{3}X[0-9]{3}','g')",
	       true, regularExpression.global);

new TestCase ( SECTION,
	       "ignoreCase of (compile '[0-9]{3}X[0-9]{3}','g')",
	       false, regularExpression.ignoreCase);


test();
