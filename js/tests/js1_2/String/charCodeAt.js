





































gTestfile = 'charCodeAt.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'String:charCodeAt';

writeHeaderToLog('Executing script: charCodeAt.js');
writeHeaderToLog( SECTION + " "+ TITLE);

var aString = new String("tEs5");

new TestCase( SECTION, "aString.charCodeAt(-2)", NaN, aString.charCodeAt(-2));
new TestCase( SECTION, "aString.charCodeAt(-1)", NaN, aString.charCodeAt(-1));
new TestCase( SECTION, "aString.charCodeAt( 0)", 116, aString.charCodeAt( 0));
new TestCase( SECTION, "aString.charCodeAt( 1)",  69, aString.charCodeAt( 1));
new TestCase( SECTION, "aString.charCodeAt( 2)", 115, aString.charCodeAt( 2));
new TestCase( SECTION, "aString.charCodeAt( 3)",  53, aString.charCodeAt( 3));
new TestCase( SECTION, "aString.charCodeAt( 4)", NaN, aString.charCodeAt( 4));
new TestCase( SECTION, "aString.charCodeAt( 5)", NaN, aString.charCodeAt( 5));
new TestCase( SECTION, "aString.charCodeAt( Infinity)", NaN, aString.charCodeAt( Infinity));
new TestCase( SECTION, "aString.charCodeAt(-Infinity)", NaN, aString.charCodeAt(-Infinity));


test();

