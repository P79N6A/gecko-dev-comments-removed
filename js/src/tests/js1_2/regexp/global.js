





































gTestfile = 'global.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'RegExp: global';

writeHeaderToLog('Executing script: global.js');
writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase ( SECTION, "/xyz/g.global",
	       true, /xyz/g.global);


new TestCase ( SECTION, "/xyz/.global",
	       false, /xyz/.global);


new TestCase ( SECTION, "'123 456 789'.match(/\\d+/g)",
	       String(["123","456","789"]), String('123 456 789'.match(/\d+/g)));


new TestCase ( SECTION, "'123 456 789'.match(/(\\d+)/g)",
	       String(["123","456","789"]), String('123 456 789'.match(/(\d+)/g)));


new TestCase ( SECTION, "'123 456 789'.match(/\\d+/)",
	       String(["123"]), String('123 456 789'.match(/\d+/)));


new TestCase ( SECTION, "(new RegExp('[a-z]','g')).global",
	       true, (new RegExp('[a-z]','g')).global);


new TestCase ( SECTION, "(new RegExp('[a-z]','i')).global",
	       false, (new RegExp('[a-z]','i')).global);


new TestCase ( SECTION, "'123 456 789'.match(new RegExp('\\\\d+','g'))",
	       String(["123","456","789"]), String('123 456 789'.match(new RegExp('\\d+','g'))));


new TestCase ( SECTION, "'123 456 789'.match(new RegExp('(\\\\d+)','g'))",
	       String(["123","456","789"]), String('123 456 789'.match(new RegExp('(\\d+)','g'))));


new TestCase ( SECTION, "'123 456 789'.match(new RegExp('\\\\d+','i'))",
	       String(["123"]), String('123 456 789'.match(new RegExp('\\d+','i'))));

test();
