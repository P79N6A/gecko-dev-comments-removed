





































gTestfile = 'RegExp_object.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: object';

writeHeaderToLog('Executing script: RegExp_object.js');
writeHeaderToLog( SECTION + " "+ TITLE);

var SSN_pattern = new RegExp("\\d{3}-\\d{2}-\\d{4}");


new TestCase ( SECTION, "'Test SSN is 123-34-4567'.match(SSN_pattern))",
	       String(["123-34-4567"]), String('Test SSN is 123-34-4567'.match(SSN_pattern)));


new TestCase ( SECTION, "'Test SSN is 123-34-4567'.match(SSN_pattern))",
	       String(["123-34-4567"]), String('Test SSN is 123-34-4567'.match(SSN_pattern)));

var PHONE_pattern = new RegExp("\\(?(\\d{3})\\)?-?(\\d{3})-(\\d{4})");

new TestCase ( SECTION, "'Our phone number is (408)345-2345.'.match(PHONE_pattern))",
	       String(["(408)345-2345","408","345","2345"]), String('Our phone number is (408)345-2345.'.match(PHONE_pattern)));


new TestCase ( SECTION, "'The phone number is 408-345-2345!'.match(PHONE_pattern))",
	       String(["408-345-2345","408","345","2345"]), String('The phone number is 408-345-2345!'.match(PHONE_pattern)));


new TestCase ( SECTION, "String(PHONE_pattern.toString())",
	       "/\\(?(\\d{3})\\)?-?(\\d{3})-(\\d{4})/", String(PHONE_pattern.toString()));


new TestCase ( SECTION, "PHONE_pattern + ' is the string'",
	       "/\\(?(\\d{3})\\)?-?(\\d{3})-(\\d{4})/ is the string",PHONE_pattern + ' is the string');


new TestCase ( SECTION, "SSN_pattern - 8",
	       NaN,SSN_pattern - 8);

var testPattern = new RegExp("(\\d+)45(\\d+)90");

test();
