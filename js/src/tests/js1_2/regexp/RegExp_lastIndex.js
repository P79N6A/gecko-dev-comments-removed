





































gTestfile = 'RegExp_lastIndex.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
var TITLE   = 'RegExp: lastIndex';
var BUGNUMBER="123802";

startTest();
writeHeaderToLog('Executing script: RegExp_lastIndex.js');
writeHeaderToLog( SECTION + " "+ TITLE);


re=/x./g;
re.lastIndex=4;
new TestCase ( SECTION, "re=/x./g; re.lastIndex=4; re.exec('xyabcdxa')",
	       '["xa"]', String(re.exec('xyabcdxa')));


new TestCase ( SECTION, "re.lastIndex",
	       8, re.lastIndex);


new TestCase ( SECTION, "re.exec('xyabcdef')",
	       null, re.exec('xyabcdef'));


new TestCase ( SECTION, "re.lastIndex",
	       0, re.lastIndex);


new TestCase ( SECTION, "re.exec('xyabcdef')",
	       '["xy"]', String(re.exec('xyabcdef')));


re.lastIndex=30;
new TestCase ( SECTION, "re.lastIndex=30; re.exec('123xaxbxc456')",
	       null, re.exec('123xaxbxc456'));

test();
