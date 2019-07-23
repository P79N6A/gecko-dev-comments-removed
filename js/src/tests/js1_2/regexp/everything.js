





































gTestfile = 'everything.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE   = 'RegExp';

writeHeaderToLog('Executing script: everything.js');
writeHeaderToLog( SECTION + " "+ TITLE);



new TestCase ( SECTION, "'Sally and Fred are sure to come'.match(/^[a-z\\s]*/i)",
	       String(["Sally and Fred are sure to come"]), String('Sally and Fred are sure to come'.match(/^[a-z\s]*/i)));


new TestCase ( SECTION, "'test123W+xyz'.match(new RegExp('^[a-z]*[0-9]+[A-Z]?.(123|xyz)$'))",
	       String(["test123W+xyz","xyz"]), String('test123W+xyz'.match(new RegExp('^[a-z]*[0-9]+[A-Z]?.(123|xyz)$'))));


new TestCase ( SECTION, "'number one 12365 number two 9898'.match(/(\d+)\D+(\d+)/)",
	       String(["12365 number two 9898","12365","9898"]), String('number one 12365 number two 9898'.match(/(\d+)\D+(\d+)/)));

var simpleSentence = /(\s?[^\!\?\.]+[\!\?\.])+/;

new TestCase ( SECTION, "'See Spot run.'.match(simpleSentence)",
	       String(["See Spot run.","See Spot run."]), String('See Spot run.'.match(simpleSentence)));


new TestCase ( SECTION, "'I like it. What's up? I said NO!'.match(simpleSentence)",
	       String(["I like it. What's up? I said NO!",' I said NO!']), String('I like it. What\'s up? I said NO!'.match(simpleSentence)));


new TestCase ( SECTION, "'the quick brown fox jumped over the lazy dogs'.match(/((\\w+)\\s*)+/)",
	       String(['the quick brown fox jumped over the lazy dogs','dogs','dogs']),String('the quick brown fox jumped over the lazy dogs'.match(/((\w+)\s*)+/)));

test();
