





































gTestfile = 'RegExp_lastParen_as_array.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: $+';

writeHeaderToLog('Executing script: RegExp_lastParen_as_array.js');
writeHeaderToLog( SECTION + " "+ TITLE);


'abcd'.match(/(abc)d/);
new TestCase ( SECTION, "'abcd'.match(/(abc)d/); RegExp['$+']",
	       'abc', RegExp['$+']);


'abcd'.match(/(bcd)e/);
new TestCase ( SECTION, "'abcd'.match(/(bcd)e/); RegExp['$+']",
	       'abc', RegExp['$+']);


'abcdefg'.match(/(a(b(c(d)e)f)g)/);
new TestCase ( SECTION, "'abcdefg'.match(/(a(b(c(d)e)f)g)/); RegExp['$+']",
	       'd', RegExp['$+']);


'abcdefg'.match(new RegExp('(a(b(c(d)e)f)g)'));
new TestCase ( SECTION, "'abcdefg'.match(new RegExp('(a(b(c(d)e)f)g)')); RegExp['$+']",
	       'd', RegExp['$+']);


'abcdefg'.match(/(a(b)c)(d(e)f)/);
new TestCase ( SECTION, "'abcdefg'.match(/(a(b)c)(d(e)f)/); RegExp['$+']",
	       'e', RegExp['$+']);


'abcdefg'.match(/(^)abc/);
new TestCase ( SECTION, "'abcdefg'.match(/(^)abc/); RegExp['$+']",
	       '', RegExp['$+']);


'abcdefg'.match(/(^a)bc/);
new TestCase ( SECTION, "'abcdefg'.match(/(^a)bc/); RegExp['$+']",
	       'a', RegExp['$+']);


'abcdefg'.match(new RegExp('(^a)bc'));
new TestCase ( SECTION, "'abcdefg'.match(new RegExp('(^a)bc')); RegExp['$+']",
	       'a', RegExp['$+']);


'abcdefg'.match(/bc/);
new TestCase ( SECTION, "'abcdefg'.match(/bc/); RegExp['$+']",
	       '', RegExp['$+']);

test();
