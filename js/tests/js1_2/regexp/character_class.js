





































gTestfile = 'character_class.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'RegExp: []';

writeHeaderToLog('Executing script: character_class.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "'abcde'.match(new RegExp('ab[ercst]de'))",
	       String(["abcde"]), String('abcde'.match(new RegExp('ab[ercst]de'))));


new TestCase ( SECTION, "'abcde'.match(new RegExp('ab[erst]de'))",
	       null, 'abcde'.match(new RegExp('ab[erst]de')));


new TestCase ( SECTION, "'abcdefghijkl'.match(new RegExp('[d-h]+'))",
	       String(["defgh"]), String('abcdefghijkl'.match(new RegExp('[d-h]+'))));


new TestCase ( SECTION, "'abc6defghijkl'.match(new RegExp('[1234567].{2}'))",
	       String(["6de"]), String('abc6defghijkl'.match(new RegExp('[1234567].{2}'))));


new TestCase ( SECTION, "'\n\n\abc324234\n'.match(new RegExp('[a-c\\d]+'))",
	       String(["abc324234"]), String('\n\n\abc324234\n'.match(new RegExp('[a-c\\d]+'))));


new TestCase ( SECTION, "'abc'.match(new RegExp('ab[.]?c'))",
	       String(["abc"]), String('abc'.match(new RegExp('ab[.]?c'))));


new TestCase ( SECTION, "'abc'.match(new RegExp('a[b]c'))",
	       String(["abc"]), String('abc'.match(new RegExp('a[b]c'))));


new TestCase ( SECTION, "'a1b  b2c  c3d  def  f4g'.match(new RegExp('[a-z][^1-9][a-z]'))",
	       String(["def"]), String('a1b  b2c  c3d  def  f4g'.match(new RegExp('[a-z][^1-9][a-z]'))));


new TestCase ( SECTION, "'123*&$abc'.match(new RegExp('[*&$]{3}'))",
	       String(["*&$"]), String('123*&$abc'.match(new RegExp('[*&$]{3}'))));


new TestCase ( SECTION, "'abc'.match(new RegExp('a[^1-9]c'))",
	       String(["abc"]), String('abc'.match(new RegExp('a[^1-9]c'))));


new TestCase ( SECTION, "'abc'.match(new RegExp('a[^b]c'))",
	       null, 'abc'.match(new RegExp('a[^b]c')));


new TestCase ( SECTION, "'abc#$%def%&*@ghi)(*&'.match(new RegExp('[^a-z]{4}'))",
	       String(["%&*@"]), String('abc#$%def%&*@ghi)(*&'.match(new RegExp('[^a-z]{4}'))));


new TestCase ( SECTION, "'abc#$%def%&*@ghi)(*&'.match(/[^a-z]{4}/)",
	       String(["%&*@"]), String('abc#$%def%&*@ghi)(*&'.match(/[^a-z]{4}/)));

test();
