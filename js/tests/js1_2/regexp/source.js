





































gTestfile = 'source.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'RegExp: source';

writeHeaderToLog('Executing script: source.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "/xyz/g.source",
	       "xyz", /xyz/g.source);


new TestCase ( SECTION, "/xyz/.source",
	       "xyz", /xyz/.source);


new TestCase ( SECTION, "/abc\\\\def/.source",
	       "abc\\\\def", /abc\\def/.source);


new TestCase ( SECTION, "/abc[\\b]def/.source",
	       "abc[\\b]def", /abc[\b]def/.source);


new TestCase ( SECTION, "(new RegExp('xyz')).source",
	       "xyz", (new RegExp('xyz')).source);


new TestCase ( SECTION, "(new RegExp('xyz','g')).source",
	       "xyz", (new RegExp('xyz','g')).source);


new TestCase ( SECTION, "(new RegExp('abc\\\\\\\\def')).source",
	       "abc\\\\def", (new RegExp('abc\\\\def')).source);


new TestCase ( SECTION, "(new RegExp('abc[\\\\b]def')).source",
	       "abc[\\b]def", (new RegExp('abc[\\b]def')).source);

test();
