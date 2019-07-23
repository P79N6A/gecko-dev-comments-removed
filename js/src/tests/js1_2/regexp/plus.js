





































gTestfile = 'plus.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: +';

writeHeaderToLog('Executing script: plus.js');
writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase ( SECTION, "'abcdddddefg'.match(new RegExp('d+'))",
	       String(["ddddd"]), String('abcdddddefg'.match(new RegExp('d+'))));


new TestCase ( SECTION, "'abcdefg'.match(new RegExp('o+'))",
	       null, 'abcdefg'.match(new RegExp('o+')));


new TestCase ( SECTION, "'abcdefg'.match(new RegExp('d+'))",
	       String(['d']), String('abcdefg'.match(new RegExp('d+'))));


new TestCase ( SECTION, "'abbbbbbbc'.match(new RegExp('(b+)(b+)(b+)'))",
	       String(["bbbbbbb","bbbbb","b","b"]), String('abbbbbbbc'.match(new RegExp('(b+)(b+)(b+)'))));


new TestCase ( SECTION, "'abbbbbbbc'.match(new RegExp('(b+)(b*)'))",
	       String(["bbbbbbb","bbbbbbb",""]), String('abbbbbbbc'.match(new RegExp('(b+)(b*)'))));


new TestCase ( SECTION, "'abbbbbbbc'.match(new RegExp('b*b+'))",
	       String(['bbbbbbb']), String('abbbbbbbc'.match(new RegExp('b*b+'))));


new TestCase ( SECTION, "'abbbbbbbc'.match(/(b+)(b*)/)",
	       String(["bbbbbbb","bbbbbbb",""]), String('abbbbbbbc'.match(/(b+)(b*)/)));


new TestCase ( SECTION, "'abbbbbbbc'.match(/b*b+/)",
	       String(['bbbbbbb']), String('abbbbbbbc'.match(/b*b+/)));

test();
