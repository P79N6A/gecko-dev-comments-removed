





































gTestfile = 'concat.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'String:concat';

writeHeaderToLog('Executing script: concat.js');
writeHeaderToLog( SECTION + " "+ TITLE);

var aString = new String("test string");
var bString = new String(" another ");

new TestCase( SECTION, "aString.concat(' more')", "test string more",     aString.concat(' more').toString());
new TestCase( SECTION, "aString.concat(bString)", "test string another ", aString.concat(bString).toString());
new TestCase( SECTION, "aString                ", "test string",          aString.toString());
new TestCase( SECTION, "bString                ", " another ",            bString.toString());
new TestCase( SECTION, "aString.concat(345)    ", "test string345",       aString.concat(345).toString());
new TestCase( SECTION, "aString.concat(true)   ", "test stringtrue",      aString.concat(true).toString());
new TestCase( SECTION, "aString.concat(null)   ", "test stringnull",      aString.concat(null).toString());
new TestCase( SECTION, "aString.concat([])     ", "test string[]",          aString.concat([]).toString());
new TestCase( SECTION, "aString.concat([1,2,3])", "test string[1, 2, 3]",     aString.concat([1,2,3]).toString());

new TestCase( SECTION, "'abcde'.concat(' more')", "abcde more",     'abcde'.concat(' more').toString());
new TestCase( SECTION, "'abcde'.concat(bString)", "abcde another ", 'abcde'.concat(bString).toString());
new TestCase( SECTION, "'abcde'                ", "abcde",          'abcde');
new TestCase( SECTION, "'abcde'.concat(345)    ", "abcde345",       'abcde'.concat(345).toString());
new TestCase( SECTION, "'abcde'.concat(true)   ", "abcdetrue",      'abcde'.concat(true).toString());
new TestCase( SECTION, "'abcde'.concat(null)   ", "abcdenull",      'abcde'.concat(null).toString());
new TestCase( SECTION, "'abcde'.concat([])     ", "abcde[]",          'abcde'.concat([]).toString());
new TestCase( SECTION, "'abcde'.concat([1,2,3])", "abcde[1, 2, 3]",     'abcde'.concat([1,2,3]).toString());


new TestCase( SECTION, "'abcde'.concat()       ", "abcde",          'abcde'.concat().toString());

test();

