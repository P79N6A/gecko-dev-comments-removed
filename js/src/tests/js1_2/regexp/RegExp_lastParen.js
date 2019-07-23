





































gTestfile = 'RegExp_lastParen.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: lastParen';

writeHeaderToLog('Executing script: RegExp_lastParen.js');
writeHeaderToLog( SECTION + " "+ TITLE);


'abcd'.match(/(abc)d/);
new TestCase ( SECTION, "'abcd'.match(/(abc)d/); RegExp.lastParen",
	       'abc', RegExp.lastParen);


'abcd'.match(new RegExp('(abc)d'));
new TestCase ( SECTION, "'abcd'.match(new RegExp('(abc)d')); RegExp.lastParen",
	       'abc', RegExp.lastParen);


'abcd'.match(/(bcd)e/);
new TestCase ( SECTION, "'abcd'.match(/(bcd)e/); RegExp.lastParen",
	       'abc', RegExp.lastParen);


'abcdefg'.match(/(a(b(c(d)e)f)g)/);
new TestCase ( SECTION, "'abcdefg'.match(/(a(b(c(d)e)f)g)/); RegExp.lastParen",
	       'd', RegExp.lastParen);


'abcdefg'.match(/(a(b)c)(d(e)f)/);
new TestCase ( SECTION, "'abcdefg'.match(/(a(b)c)(d(e)f)/); RegExp.lastParen",
	       'e', RegExp.lastParen);


'abcdefg'.match(/(^)abc/);
new TestCase ( SECTION, "'abcdefg'.match(/(^)abc/); RegExp.lastParen",
	       '', RegExp.lastParen);


'abcdefg'.match(/(^a)bc/);
new TestCase ( SECTION, "'abcdefg'.match(/(^a)bc/); RegExp.lastParen",
	       'a', RegExp.lastParen);


'abcdefg'.match(new RegExp('(^a)bc'));
new TestCase ( SECTION, "'abcdefg'.match(new RegExp('(^a)bc')); RegExp.lastParen",
	       'a', RegExp.lastParen);


'abcdefg'.match(/bc/);
new TestCase ( SECTION, "'abcdefg'.match(/bc/); RegExp.lastParen",
	       '', RegExp.lastParen);

test();
