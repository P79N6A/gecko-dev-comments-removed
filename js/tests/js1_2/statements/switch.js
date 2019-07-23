





































gTestfile = 'switch.js';











var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
var TITLE   = 'statements: switch';
var BUGNUMBER="323696";

startTest();
writeHeaderToLog("Executing script: switch.js");
writeHeaderToLog( SECTION + " "+ TITLE);


var var1 = "match string";
var match1 = false;
var match2 = false;
var match3 = false;

switch (var1)
{
case "match string":
  match1 = true;
case "bad string 1":
  match2 = true;
  break;
case "bad string 2":
  match3 = true;
}

new TestCase ( SECTION, 'switch statement',
	       true, match1);

new TestCase ( SECTION, 'switch statement',
	       true, match2);

new TestCase ( SECTION, 'switch statement',
	       false, match3);

var var2 = 3;

var match1 = false;
var match2 = false;
var match3 = false;
var match4 = false;
var match5 = false;

switch (var2)
{
case 1:









  match3 = true;
  break;
case 2:
  match4 = true;
  break;
case 3:
  match5 = true;
  break;
}
new TestCase ( SECTION, 'switch statement',
	       false, match1);

new TestCase ( SECTION, 'switch statement',
	       false, match2);

new TestCase ( SECTION, 'switch statement',
	       false, match3);

new TestCase ( SECTION, 'switch statement',
	       false, match4);

new TestCase ( SECTION, 'switch statement',
	       true, match5);

test();
