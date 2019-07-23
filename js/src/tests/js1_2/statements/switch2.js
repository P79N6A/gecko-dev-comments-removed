





































gTestfile = 'switch2.js';











var SECTION = 'As described in Netscape doc "Whats new in JavaScript 1.2"';
var VERSION = 'no version';
var TITLE   = 'statements: switch';
var BUGNUMBER="323626";

startTest();
writeHeaderToLog("Executing script: switch2.js");
writeHeaderToLog( SECTION + " "+ TITLE);



function f0(i) {
  switch(i) {
  default:
  case "a":
  case "b":
    return "ab*"
      case "c":
    return "c";
  case "d":
    return "d";
  }
  return "";
}
new TestCase(SECTION, 'switch statement',
	     f0("a"), "ab*");

new TestCase(SECTION, 'switch statement',
	     f0("b"), "ab*");

new TestCase(SECTION, 'switch statement',
	     f0("*"), "ab*");

new TestCase(SECTION, 'switch statement',
	     f0("c"), "c");

new TestCase(SECTION, 'switch statement',
	     f0("d"), "d");

function f1(i) {
  switch(i) {
  case "a":
  case "b":
  default:
    return "ab*"
      case "c":
    return "c";
  case "d":
    return "d";
  }
  return "";
}

new TestCase(SECTION, 'switch statement',
	     f1("a"), "ab*");

new TestCase(SECTION, 'switch statement',
	     f1("b"), "ab*");

new TestCase(SECTION, 'switch statement',
	     f1("*"), "ab*");

new TestCase(SECTION, 'switch statement',
	     f1("c"), "c");

new TestCase(SECTION, 'switch statement',
	     f1("d"), "d");


function f2(i) {
  switch (i) {
  case 0:
  case 1:
    return 1;
  case 2:
    return 2;
  }
  
  return 3;
}

new TestCase(SECTION, 'switch statement',
	     f2(0), 1);

new TestCase(SECTION, 'switch statement',
	     f2(1), 1);

new TestCase(SECTION, 'switch statement',
	     f2(2), 2);

new TestCase(SECTION, 'switch statement',
	     f2(3), 3);


var se = 0;
switch (se = 1) {
}
new TestCase(SECTION, 'switch statement',
	     se, 1);


se = 0;
switch (se) {
default:
  se = 1;
}
new TestCase(SECTION, 'switch statement',
	     se, 1);


se = 0;
for (var i=0; i < 2; i++) {
  switch (i) {
  case 0:
  case 1:
    break;
  }
  se = 1;
}
new TestCase(SECTION, 'switch statement',
	     se, 1);


se = 0;
i = 0;
switch (i) {
case 0:
  se++;
  
case 1:
  se++;
  break;
}
new TestCase(SECTION, 'switch statement',
	     se, 2);
print("hi");

test();




