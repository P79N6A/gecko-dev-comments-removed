





































gTestfile = 'RegExp_input.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: input';

writeHeaderToLog('Executing script: RegExp_input.js');
writeHeaderToLog( SECTION + " "+ TITLE);

RegExp.input = "abcd12357efg";


RegExp.input = "abcd12357efg";
new TestCase ( SECTION, "RegExp.input = 'abcd12357efg'; RegExp.input",
	       "abcd12357efg", RegExp.input);


RegExp.input = "abcd12357efg";
new TestCase ( SECTION, "RegExp.input = 'abcd12357efg'; /\\d+/.exec('2345')",
	       String(["2345"]), String(/\d+/.exec('2345')));


RegExp.input = "abcd12357efg";
new TestCase ( SECTION, "RegExp.input = 'abcd12357efg'; /\\d+/.exec()",
	       String(["12357"]), String(/\d+/.exec()));


RegExp.input = "abcd12357efg";
new TestCase ( SECTION, "RegExp.input = 'abcd12357efg'; /[h-z]+/.exec()",
	       null, /[h-z]+/.exec());


RegExp.input = "abcd12357efg";
new TestCase ( SECTION, "RegExp.input = 'abcd12357efg'; /\\d+/.test('2345')",
	       true, /\d+/.test('2345'));


RegExp.input = "abcd12357efg";
new TestCase ( SECTION, "RegExp.input = 'abcd12357efg'; /\\d+/.test()",
	       true, /\d+/.test());


RegExp.input = "abcd12357efg";
new TestCase ( SECTION, "RegExp.input = 'abcd12357efg'; (new RegExp('d+')).test()",
	       true, (new RegExp('d+')).test());


RegExp.input = "abcd12357efg";
new TestCase ( SECTION, "RegExp.input = 'abcd12357efg'; /[h-z]+/.test()",
	       false, /[h-z]+/.test());


RegExp.input = "abcd12357efg";
new TestCase ( SECTION, "RegExp.input = 'abcd12357efg'; (new RegExp('[h-z]+')).test()",
	       false, (new RegExp('[h-z]+')).test());

test();
