





































gTestfile = 'string_replace.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'String: replace';

writeHeaderToLog('Executing script: string_replace.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "'adddb'.replace(/ddd/,'XX')",
	       "aXXb", 'adddb'.replace(/ddd/,'XX'));


new TestCase ( SECTION, "'adddb'.replace(/eee/,'XX')",
	       'adddb', 'adddb'.replace(/eee/,'XX'));


new TestCase ( SECTION, "'34 56 78b 12'.replace(new RegExp('[0-9]+b'),'**')",
	       "34 56 ** 12", '34 56 78b 12'.replace(new RegExp('[0-9]+b'),'**'));


new TestCase ( SECTION, "'34 56 78b 12'.replace(new RegExp('[0-9]+c'),'XX')",
	       "34 56 78b 12", '34 56 78b 12'.replace(new RegExp('[0-9]+c'),'XX'));


new TestCase ( SECTION, "'original'.replace(new RegExp(),'XX')",
	       "XXoriginal", 'original'.replace(new RegExp(),'XX'));


new TestCase ( SECTION, "'qwe ert x\t\n 345654AB'.replace(new RegExp('x\\s*\\d+(..)$'),'****')",
	       "qwe ert ****", 'qwe ert x\t\n 345654AB'.replace(new RegExp('x\\s*\\d+(..)$'),'****'));






var stringA = "abcdef";
var stringB = "ghijk";
var stringC = "zzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzzz";
stringC += stringC;
stringC += stringC;
stringC[0]; 
var stringD = "lmn";

new TestCase ( SECTION, "(stringA + stringB + stringC).replace('aa', '')",
           stringA + stringB + stringC, (stringA + stringB + stringC).replace('aa', ''));

new TestCase ( SECTION, "(stringA + stringB + stringC).replace('abc', 'AA')",
           "AAdefghijk" + stringC, (stringA + stringB + stringC).replace('abc', 'AA'));

new TestCase ( SECTION, "(stringA + stringB + stringC).replace('def', 'AA')",
           "abcAAghijk" + stringC, (stringA + stringB + stringC).replace('def', 'AA'));

new TestCase ( SECTION, "(stringA + stringB + stringC).replace('efg', 'AA')",
           "abcdAAhijk" + stringC, (stringA + stringB + stringC).replace('efg', 'AA'));

new TestCase ( SECTION, "(stringA + stringB + stringC).replace('fgh', 'AA')",
           "abcdeAAijk" + stringC, (stringA + stringB + stringC).replace('fgh', 'AA'));

new TestCase ( SECTION, "(stringA + stringB + stringC).replace('ghi', 'AA')",
           "abcdefAAjk" + stringC, (stringA + stringB + stringC).replace('ghi', 'AA'));

new TestCase ( SECTION, "(stringC + stringD).replace('lmn', 'AA')",
           stringC + "AA", (stringC + stringD).replace('lmn', 'AA'));

new TestCase ( SECTION, "(stringC + stringD).replace('lmno', 'AA')",
           stringC + stringD, (stringC + stringD).replace('lmno', 'AA'));

new TestCase ( SECTION, "(stringC + stringD).replace('mn', 'AA')",
           stringC + "lAA", (stringC + stringD).replace('mn', 'AA'));

new TestCase ( SECTION, "(stringC + stringD).replace('n', 'AA')",
           stringC + "lmAA", (stringC + stringD).replace('n', 'AA'));


test();
