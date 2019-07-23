





































gTestfile = 'RegExp_lastMatch.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: lastMatch';

writeHeaderToLog('Executing script: RegExp_lastMatch.js');
writeHeaderToLog( SECTION + " "+ TITLE);



'foo'.match(/foo/);
new TestCase ( SECTION, "'foo'.match(/foo/); RegExp.lastMatch",
	       'foo', RegExp.lastMatch);


'foo'.match(new RegExp('foo'));
new TestCase ( SECTION, "'foo'.match(new RegExp('foo')); RegExp.lastMatch",
	       'foo', RegExp.lastMatch);


'xxx'.match(/bar/);
new TestCase ( SECTION, "'xxx'.match(/bar/); RegExp.lastMatch",
	       'foo', RegExp.lastMatch);


'xxx'.match(/$/);
new TestCase ( SECTION, "'xxx'.match(/$/); RegExp.lastMatch",
	       '', RegExp.lastMatch);


'abcdefg'.match(/^..(cd)[a-z]+/);
new TestCase ( SECTION, "'abcdefg'.match(/^..(cd)[a-z]+/); RegExp.lastMatch",
	       'abcdefg', RegExp.lastMatch);


'abcdefgabcdefg'.match(/(a(b(c(d)e)f)g)\1/);
new TestCase ( SECTION, "'abcdefgabcdefg'.match(/(a(b(c(d)e)f)g)\\1/); RegExp.lastMatch",
	       'abcdefgabcdefg', RegExp.lastMatch);

test();
