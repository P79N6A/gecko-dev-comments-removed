













































var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: input';

writeHeaderToLog('Executing script: RegExp_input.js');
writeHeaderToLog( SECTION + " "+ TITLE);

var count = 0;
var testcases = new Array();

RegExp['$_'] = "abcd12357efg";


RegExp['$_'] = "abcd12357efg";
new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; RegExp['$_']",
	       "abcd12357efg", RegExp['$_']);


RegExp['$_'] = "abcd12357efg";
new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; /\\d+/.exec('2345')",
	       String(["2345"]), String(/\d+/.exec('2345')));


RegExp['$_'] = "abcd12357efg";
new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; /\\d+/.exec()",
	       String(["12357"]), String(/\d+/.exec()));


RegExp['$_'] = "abcd12357efg";
new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; /[h-z]+/.exec()",
	       null, /[h-z]+/.exec());


RegExp['$_'] = "abcd12357efg";
new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; /\\d+/.test('2345')",
	       true, /\d+/.test('2345'));


RegExp['$_'] = "abcd12357efg";
new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; /\\d+/.test()",
	       true, /\d+/.test());


RegExp['$_'] = "abcd12357efg";
new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; /[h-z]+/.test()",
	       false, /[h-z]+/.test());


RegExp['$_'] = "abcd12357efg";
new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; (new RegExp('\d+')).test()",
	       true, (new RegExp('\d+')).test());


RegExp['$_'] = "abcd12357efg";
new TestCase ( SECTION, "RegExp['$_'] = 'abcd12357efg'; (new RegExp('[h-z]+')).test()",
	       false, (new RegExp('[h-z]+')).test());

test();
