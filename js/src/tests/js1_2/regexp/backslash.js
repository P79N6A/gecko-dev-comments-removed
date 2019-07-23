





































gTestfile = 'backslash.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'RegExp: \\';

writeHeaderToLog('Executing script: backslash.js');
writeHeaderToLog( SECTION + " "+ TITLE);


new TestCase ( SECTION, "'abcde'.match(new RegExp('\e'))",
	       String(["e"]), String('abcde'.match(new RegExp('\e'))));


new TestCase ( SECTION, "'ab\\cde'.match(new RegExp('\\\\'))",
	       String(["\\"]), String('ab\\cde'.match(new RegExp('\\\\'))));


new TestCase ( SECTION, "'ab\\cde'.match(/\\\\/)",
	       String(["\\"]), String('ab\\cde'.match(/\\/)));


new TestCase ( SECTION,
	       "'before ^$*+?.()|{}[] after'.match(new RegExp('\\^\\$\\*\\+\\?\\.\\(\\)\\|\\{\\}\\[\\]'))",
	       String(["^$*+?.()|{}[]"]),
	       String('before ^$*+?.()|{}[] after'.match(new RegExp('\\^\\$\\*\\+\\?\\.\\(\\)\\|\\{\\}\\[\\]'))));


new TestCase ( SECTION,
	       "'before ^$*+?.()|{}[] after'.match(/\\^\\$\\*\\+\\?\\.\\(\\)\\|\\{\\}\\[\\]/)",
	       String(["^$*+?.()|{}[]"]),
	       String('before ^$*+?.()|{}[] after'.match(/\^\$\*\+\?\.\(\)\|\{\}\[\]/)));

test();
