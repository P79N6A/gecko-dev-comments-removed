





































gTestfile = 'RegExp_lastMatch_as_array.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: $&';

writeHeaderToLog('Executing script: RegExp_lastMatch_as_array.js');
writeHeaderToLog( SECTION + " "+ TITLE);



'foo'.match(/foo/);
new TestCase ( SECTION, "'foo'.match(/foo/); RegExp['$&']",
	       'foo', RegExp['$&']);


'foo'.match(new RegExp('foo'));
new TestCase ( SECTION, "'foo'.match(new RegExp('foo')); RegExp['$&']",
	       'foo', RegExp['$&']);


'xxx'.match(/bar/);
new TestCase ( SECTION, "'xxx'.match(/bar/); RegExp['$&']",
	       'foo', RegExp['$&']);


'xxx'.match(/$/);
new TestCase ( SECTION, "'xxx'.match(/$/); RegExp['$&']",
	       '', RegExp['$&']);


'abcdefg'.match(/^..(cd)[a-z]+/);
new TestCase ( SECTION, "'abcdefg'.match(/^..(cd)[a-z]+/); RegExp['$&']",
	       'abcdefg', RegExp['$&']);


'abcdefgabcdefg'.match(/(a(b(c(d)e)f)g)\1/);
new TestCase ( SECTION, "'abcdefgabcdefg'.match(/(a(b(c(d)e)f)g)\\1/); RegExp['$&']",
	       'abcdefgabcdefg', RegExp['$&']);

test();
