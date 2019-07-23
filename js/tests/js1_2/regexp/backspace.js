





































gTestfile = 'backspace.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'RegExp: [\b]';

writeHeaderToLog('Executing script: backspace.js');
writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase ( SECTION, "'abc\bdef'.match(new RegExp('.[\\b].'))",
	       String(["c\bd"]), String('abc\bdef'.match(new RegExp('.[\\b].'))));


new TestCase ( SECTION, "'abc\\bdef'.match(new RegExp('.[\\b].'))",
	       null, 'abc\\bdef'.match(new RegExp('.[\\b].')));


new TestCase ( SECTION, "'abc\b\b\bdef'.match(new RegExp('c[\\b]{3}d'))",
	       String(["c\b\b\bd"]), String('abc\b\b\bdef'.match(new RegExp('c[\\b]{3}d'))));


new TestCase ( SECTION, "'abc\bdef'.match(new RegExp('[^\\[\\b\\]]+'))",
	       String(["abc"]), String('abc\bdef'.match(new RegExp('[^\\[\\b\\]]+'))));


new TestCase ( SECTION, "'abcdef'.match(new RegExp('[^\\[\\b\\]]+'))",
	       String(["abcdef"]), String('abcdef'.match(new RegExp('[^\\[\\b\\]]+'))));


new TestCase ( SECTION, "'abcdef'.match(/[^\\[\\b\\]]+/)",
	       String(["abcdef"]), String('abcdef'.match(/[^\[\b\]]+/)));

test();
