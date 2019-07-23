





































gTestfile = 'equality.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'operator "=="';

writeHeaderToLog('Executing script: equality.js');
writeHeaderToLog( SECTION + " "+ TITLE);








new TestCase( SECTION, "(new String('x') == 'x')                 ",
	      false,   (new String('x') == 'x'));

new TestCase( SECTION, "('x' == new String('x'))                 ",
	      false,   ('x' == new String('x')));


test();

