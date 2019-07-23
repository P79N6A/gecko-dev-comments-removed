





































gTestfile = 'do_while.js';









var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
startTest();
var TITLE = 'statements: do_while';

writeHeaderToLog('Executing script: do_while.js');
writeHeaderToLog( SECTION + " "+ TITLE);

var done = false;
var x = 0;
do
{
  if (x++ == 3) done = true;
} while (!done);

new TestCase( SECTION, "do_while ",
	      4, x);


test();

