





































gTestfile = 'vertical_bar.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp: |';

writeHeaderToLog('Executing script: vertical_bar.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "'abc'.match(new RegExp('xyz|abc'))",
	       String(["abc"]), String('abc'.match(new RegExp('xyz|abc'))));


new TestCase ( SECTION, "'this is a test'.match(new RegExp('quiz|exam|test|homework'))",
	       String(["test"]), String('this is a test'.match(new RegExp('quiz|exam|test|homework'))));


new TestCase ( SECTION, "'abc'.match(new RegExp('xyz|...'))",
	       String(["abc"]), String('abc'.match(new RegExp('xyz|...'))));


new TestCase ( SECTION, "'abc'.match(new RegExp('(.)..|abc'))",
	       String(["abc","a"]), String('abc'.match(new RegExp('(.)..|abc'))));


new TestCase ( SECTION, "'color: grey'.match(new RegExp('.+: gr(a|e)y'))",
	       String(["color: grey","e"]), String('color: grey'.match(new RegExp('.+: gr(a|e)y'))));


new TestCase ( SECTION, "'no match'.match(new RegExp('red|white|blue'))",
	       null, 'no match'.match(new RegExp('red|white|blue')));


new TestCase ( SECTION, "'Hi Bob'.match(new RegExp('(Rob)|(Bob)|(Robert)|(Bobby)'))",
	       String(["Bob",undefined,"Bob", undefined, undefined]), String('Hi Bob'.match(new RegExp('(Rob)|(Bob)|(Robert)|(Bobby)'))));


new TestCase ( SECTION, "'abcdef'.match(new RegExp('abc|bcd|cde|def'))",
	       String(["abc"]), String('abcdef'.match(new RegExp('abc|bcd|cde|def'))));


new TestCase ( SECTION, "'Hi Bob'.match(/(Rob)|(Bob)|(Robert)|(Bobby)/)",
	       String(["Bob",undefined,"Bob", undefined, undefined]), String('Hi Bob'.match(/(Rob)|(Bob)|(Robert)|(Bobby)/)));


new TestCase ( SECTION, "'abcdef'.match(/abc|bcd|cde|def/)",
	       String(["abc"]), String('abcdef'.match(/abc|bcd|cde|def/)));

test();
