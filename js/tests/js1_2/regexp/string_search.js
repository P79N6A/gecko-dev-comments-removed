





































gTestfile = 'string_search.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'String: search';

writeHeaderToLog('Executing script: string_search.js');
writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase ( SECTION, "'abcdefg'.search(/d/)",
	       3, 'abcdefg'.search(/d/));


new TestCase ( SECTION, "'abcdefg'.search(/x/)",
	       -1, 'abcdefg'.search(/x/));


new TestCase ( SECTION, "'abcdefg123456hijklmn'.search(/\d+/)",
	       7, 'abcdefg123456hijklmn'.search(/\d+/));


new TestCase ( SECTION, "'abcdefg123456hijklmn'.search(new RegExp())",
	       0, 'abcdefg123456hijklmn'.search(new RegExp()));


new TestCase ( SECTION, "'abc'.search(new RegExp('$'))",
	       3, 'abc'.search(new RegExp('$')));


new TestCase ( SECTION, "'abc'.search(new RegExp('^'))",
	       0, 'abc'.search(new RegExp('^')));


new TestCase ( SECTION, "'abc1'.search(/.\d/)",
	       2, 'abc1'.search(/.\d/));


new TestCase ( SECTION, "'abc1'.search(/\d{2}/)",
	       -1, 'abc1'.search(/\d{2}/));

test();
