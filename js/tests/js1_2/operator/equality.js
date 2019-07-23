













































var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'operator "=="';

writeHeaderToLog('Executing script: equality.js');
writeHeaderToLog( SECTION + " "+ TITLE);

var count = 0;
var testcases = new Array();

	



	


	
new TestCase( SECTION, "(new String('x') == 'x')                 ",
	      false,   (new String('x') == 'x'));
	
new TestCase( SECTION, "('x' == new String('x'))                 ",
	      false,   ('x' == new String('x')));
	
	
test();

