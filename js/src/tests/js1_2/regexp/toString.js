





































gTestfile = 'toString.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'RegExp: toString';

writeHeaderToLog('Executing script: toString.js');
writeHeaderToLog( SECTION + " "+ TITLE);






var re = new RegExp();
new TestCase ( SECTION, "var re = new RegExp(); re.toString()",
	       '/(?:)/', re.toString());


re = /.+/;
new TestCase ( SECTION, "re = /.+/; re.toString()",
	       '/.+/', re.toString());


re = /test/gi;
new TestCase ( SECTION, "re = /test/gi; re.toString()",
	       '/test/gi', re.toString());


re = /test2/ig;
new TestCase ( SECTION, "re = /test2/ig; re.toString()",
	       '/test2/gi', re.toString());

test();
