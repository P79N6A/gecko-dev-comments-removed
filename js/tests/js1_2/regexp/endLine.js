





































gTestfile = 'endLine.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: $';

writeHeaderToLog('Executing script: endLine.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "'abcde'.match(new RegExp('de$'))",
	       String(["de"]), String('abcde'.match(new RegExp('de$'))));


new TestCase ( SECTION, "'ab\ncde'.match(new RegExp('..$e$'))",
	       null, 'ab\ncde'.match(new RegExp('..$e$')));


new TestCase ( SECTION, "'yyyyy'.match(new RegExp('xxx$'))",
	       null, 'yyyyy'.match(new RegExp('xxx$')));


new TestCase ( SECTION, "'a$$$'.match(new RegExp('\\$+$'))",
	       String(['$$$']), String('a$$$'.match(new RegExp('\\$+$'))));


new TestCase ( SECTION, "'a$$$'.match(/\\$+$/)",
	       String(['$$$']), String('a$$$'.match(/\$+$/)));

RegExp.multiline = true;

new TestCase ( SECTION, "'abc\n123xyz890\nxyz'.match(new RegExp('\\d+$'))",
	       String(['890']), String('abc\n123xyz890\nxyz'.match(new RegExp('\\d+$'))));

test();
