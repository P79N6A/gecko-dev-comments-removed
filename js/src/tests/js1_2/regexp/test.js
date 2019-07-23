





































gTestfile = 'test.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: test';

writeHeaderToLog('Executing script: test.js');
writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase ( SECTION,
	       "/[0-9]{3}/.test('23 2 34 678 9 09')",
	       true, /[0-9]{3}/.test('23 2 34 678 9 09'));

new TestCase ( SECTION,
	       "/[0-9]{3}/.test('23 2 34 78 9 09')",
	       false, /[0-9]{3}/.test('23 2 34 78 9 09'));

new TestCase ( SECTION,
	       "/\w+ \w+ \w+/.test('do a test')",
	       true, /\w+ \w+ \w+/.test("do a test"));

new TestCase ( SECTION,
	       "/\w+ \w+ \w+/.test('a test')",
	       false, /\w+ \w+ \w+/.test("a test"));

new TestCase ( SECTION,
	       "(new RegExp('[0-9]{3}')).test('23 2 34 678 9 09')",
	       true, (new RegExp('[0-9]{3}')).test('23 2 34 678 9 09'));

new TestCase ( SECTION,
	       "(new RegExp('[0-9]{3}')).test('23 2 34 78 9 09')",
	       false, (new RegExp('[0-9]{3}')).test('23 2 34 78 9 09'));

new TestCase ( SECTION,
	       "(new RegExp('\\\\w+ \\\\w+ \\\\w+')).test('do a test')",
	       true, (new RegExp('\\w+ \\w+ \\w+')).test("do a test"));

new TestCase ( SECTION,
	       "(new RegExp('\\\\w+ \\\\w+ \\\\w+')).test('a test')",
	       false, (new RegExp('\\w+ \\w+ \\w+')).test("a test"));

test();
