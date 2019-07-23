





































gTestfile = 'string_replace.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'String: replace';

writeHeaderToLog('Executing script: string_replace.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "'adddb'.replace(/ddd/,'XX')",
	       "aXXb", 'adddb'.replace(/ddd/,'XX'));


new TestCase ( SECTION, "'adddb'.replace(/eee/,'XX')",
	       'adddb', 'adddb'.replace(/eee/,'XX'));


new TestCase ( SECTION, "'34 56 78b 12'.replace(new RegExp('[0-9]+b'),'**')",
	       "34 56 ** 12", '34 56 78b 12'.replace(new RegExp('[0-9]+b'),'**'));


new TestCase ( SECTION, "'34 56 78b 12'.replace(new RegExp('[0-9]+c'),'XX')",
	       "34 56 78b 12", '34 56 78b 12'.replace(new RegExp('[0-9]+c'),'XX'));


new TestCase ( SECTION, "'original'.replace(new RegExp(),'XX')",
	       "XXoriginal", 'original'.replace(new RegExp(),'XX'));


new TestCase ( SECTION, "'qwe ert x\t\n 345654AB'.replace(new RegExp('x\\s*\\d+(..)$'),'****')",
	       "qwe ert ****", 'qwe ert x\t\n 345654AB'.replace(new RegExp('x\\s*\\d+(..)$'),'****'));


test();
