





































gTestfile = 'String.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'functions: String';

writeHeaderToLog('Executing script: String.js');
writeHeaderToLog( SECTION + " "+ TITLE);

new TestCase( SECTION, "String(true)            ",
	      'true',  (String(true)));
new TestCase( SECTION, "String(false)           ",
	      'false',  (String(false)));
new TestCase( SECTION, "String(-124)           ",
	      '-124',  (String(-124)));
new TestCase( SECTION, "String(1.23)          ",
	      '1.23',  (String(1.23)));
new TestCase( SECTION, "String({p:1})           ",
	      '{p:1}',  (String({p:1})));
new TestCase( SECTION, "String(null)            ",
	      'null',  (String(null)));
new TestCase( SECTION, "String([1,2,3])            ",
	      '[1, 2, 3]',  (String([1,2,3])));

test();

