





































gTestfile = 'RegExp_leftContext.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: leftContext';

writeHeaderToLog('Executing script: RegExp_leftContext.js');
writeHeaderToLog( SECTION + " "+ TITLE);


'abc123xyz'.match(/123/);
new TestCase ( SECTION, "'abc123xyz'.match(/123/); RegExp.leftContext",
	       'abc', RegExp.leftContext);


'abc123xyz'.match(/456/);
new TestCase ( SECTION, "'abc123xyz'.match(/456/); RegExp.leftContext",
	       'abc', RegExp.leftContext);


'abc123xyz'.match(/abc123xyz/);
new TestCase ( SECTION, "'abc123xyz'.match(/abc123xyz/); RegExp.leftContext",
	       '', RegExp.leftContext);


'xxxx'.match(/$/);
new TestCase ( SECTION, "'xxxx'.match(/$/); RegExp.leftContext",
	       'xxxx', RegExp.leftContext);


'test'.match(/^/);
new TestCase ( SECTION, "'test'.match(/^/); RegExp.leftContext",
	       '', RegExp.leftContext);


'xxxx'.match(new RegExp('$'));
new TestCase ( SECTION, "'xxxx'.match(new RegExp('$')); RegExp.leftContext",
	       'xxxx', RegExp.leftContext);


'test'.match(new RegExp('^'));
new TestCase ( SECTION, "'test'.match(new RegExp('^')); RegExp.leftContext",
	       '', RegExp.leftContext);

test();
