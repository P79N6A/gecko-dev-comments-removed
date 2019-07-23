





































gTestfile = 'RegExp_rightContext_as_array.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: $\'';

writeHeaderToLog('Executing script: RegExp_rightContext.js');
writeHeaderToLog( SECTION + " "+ TITLE);


'abc123xyz'.match(/123/);
new TestCase ( SECTION, "'abc123xyz'.match(/123/); RegExp['$\'']",
	       'xyz', RegExp['$\'']);


'abc123xyz'.match(/456/);
new TestCase ( SECTION, "'abc123xyz'.match(/456/); RegExp['$\'']",
	       'xyz', RegExp['$\'']);


'abc123xyz'.match(/abc123xyz/);
new TestCase ( SECTION, "'abc123xyz'.match(/abc123xyz/); RegExp['$\'']",
	       '', RegExp['$\'']);


'xxxx'.match(/$/);
new TestCase ( SECTION, "'xxxx'.match(/$/); RegExp['$\'']",
	       '', RegExp['$\'']);


'test'.match(/^/);
new TestCase ( SECTION, "'test'.match(/^/); RegExp['$\'']",
	       'test', RegExp['$\'']);


'xxxx'.match(new RegExp('$'));
new TestCase ( SECTION, "'xxxx'.match(new RegExp('$')); RegExp['$\'']",
	       '', RegExp['$\'']);


'test'.match(new RegExp('^'));
new TestCase ( SECTION, "'test'.match(new RegExp('^')); RegExp['$\'']",
	       'test', RegExp['$\'']);

test();
