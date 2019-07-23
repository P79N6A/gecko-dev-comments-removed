





































gTestfile = 'string_split.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'String: split';

writeHeaderToLog('Executing script: string_split.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "'a b c de f'.split(/\s/)",
	       String(["a","b","c","de","f"]), String('a b c de f'.split(/\s/)));


new TestCase ( SECTION, "'a b c de f'.split(/\s/,3)",
	       String(["a","b","c"]), String('a b c de f'.split(/\s/,3)));


new TestCase ( SECTION, "'a b c de f'.split(/X/)",
	       String(["a b c de f"]), String('a b c de f'.split(/X/)));


new TestCase ( SECTION, "'dfe23iu 34 =+65--'.split(/\d+/)",
	       String(["dfe","iu "," =+","--"]), String('dfe23iu 34 =+65--'.split(/\d+/)));


new TestCase ( SECTION, "'dfe23iu 34 =+65--'.split(new RegExp('\\d+'))",
	       String(["dfe","iu "," =+","--"]), String('dfe23iu 34 =+65--'.split(new RegExp('\\d+'))));


new TestCase ( SECTION, "'abc'.split(/[a-z]/)",
	       String(["","",""]), String('abc'.split(/[a-z]/)));


new TestCase ( SECTION, "'abc'.split(/[a-z]/)",
	       String(["","",""]), String('abc'.split(/[a-z]/)));


new TestCase ( SECTION, "'abc'.split(new RegExp('[a-z]'))",
	       String(["","",""]), String('abc'.split(new RegExp('[a-z]'))));


new TestCase ( SECTION, "'abc'.split(new RegExp('[a-z]'))",
	       String(["","",""]), String('abc'.split(new RegExp('[a-z]'))));

test();
