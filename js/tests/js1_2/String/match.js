





































gTestfile = 'match.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'String:match';

writeHeaderToLog('Executing script: match.js');
writeHeaderToLog( SECTION + " "+ TITLE);

var aString = new String("this is a test string");

new TestCase( SECTION, "aString.match(/is.*test/)  ", String(["is is a test"]), String(aString.match(/is.*test/)));
new TestCase( SECTION, "aString.match(/s.*s/)  ", String(["s is a test s"]), String(aString.match(/s.*s/)));

test();

