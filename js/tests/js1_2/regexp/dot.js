





































gTestfile = 'dot.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'RegExp: .';

writeHeaderToLog('Executing script: dot.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "'abcde'.match(new RegExp('ab.de'))",
	       String(["abcde"]), String('abcde'.match(new RegExp('ab.de'))));


new TestCase ( SECTION, "'line 1\nline 2'.match(new RegExp('.+'))",
	       String(["line 1"]), String('line 1\nline 2'.match(new RegExp('.+'))));


new TestCase ( SECTION, "'this is a test'.match(new RegExp('.*a.*'))",
	       String(["this is a test"]), String('this is a test'.match(new RegExp('.*a.*'))));


new TestCase ( SECTION, "'this is a *&^%$# test'.match(new RegExp('.+'))",
	       String(["this is a *&^%$# test"]), String('this is a *&^%$# test'.match(new RegExp('.+'))));


new TestCase ( SECTION, "'....'.match(new RegExp('.+'))",
	       String(["...."]), String('....'.match(new RegExp('.+'))));


new TestCase ( SECTION, "'abcdefghijklmnopqrstuvwxyz'.match(new RegExp('.+'))",
	       String(["abcdefghijklmnopqrstuvwxyz"]), String('abcdefghijklmnopqrstuvwxyz'.match(new RegExp('.+'))));


new TestCase ( SECTION, "'ABCDEFGHIJKLMNOPQRSTUVWXYZ'.match(new RegExp('.+'))",
	       String(["ABCDEFGHIJKLMNOPQRSTUVWXYZ"]), String('ABCDEFGHIJKLMNOPQRSTUVWXYZ'.match(new RegExp('.+'))));


new TestCase ( SECTION, "'`1234567890-=~!@#$%^&*()_+'.match(new RegExp('.+'))",
	       String(["`1234567890-=~!@#$%^&*()_+"]), String('`1234567890-=~!@#$%^&*()_+'.match(new RegExp('.+'))));


new TestCase ( SECTION, "'|\\[{]};:\"\',<>.?/'.match(new RegExp('.+'))",
	       String(["|\\[{]};:\"\',<>.?/"]), String('|\\[{]};:\"\',<>.?/'.match(new RegExp('.+'))));


new TestCase ( SECTION, "'|\\[{]};:\"\',<>.?/'.match(/.+/)",
	       String(["|\\[{]};:\"\',<>.?/"]), String('|\\[{]};:\"\',<>.?/'.match(/.+/)));

test();
