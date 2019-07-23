





































gTestfile = 'beginLine.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'RegExp: ^';

writeHeaderToLog('Executing script: beginLine.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "'abcde'.match(new RegExp('^ab'))",
	       String(["ab"]), String('abcde'.match(new RegExp('^ab'))));


new TestCase ( SECTION, "'ab\ncde'.match(new RegExp('^..^e'))",
	       null, 'ab\ncde'.match(new RegExp('^..^e')));


new TestCase ( SECTION, "'yyyyy'.match(new RegExp('^xxx'))",
	       null, 'yyyyy'.match(new RegExp('^xxx')));


new TestCase ( SECTION, "'^^^x'.match(new RegExp('^\\^+'))",
	       String(['^^^']), String('^^^x'.match(new RegExp('^\\^+'))));


new TestCase ( SECTION, "'^^^x'.match(/^\\^+/)",
	       String(['^^^']), String('^^^x'.match(/^\^+/)));

RegExp.multiline = true;

new TestCase ( SECTION, "'abc\n123xyz'.match(new RegExp('^\\d+'))",
	       String(['123']), String('abc\n123xyz'.match(new RegExp('^\\d+'))));

test();
