














































var gTestfile = 'regress-194364.js';
var UBound = 0;
var BUGNUMBER = 194364;
var summary = 'Testing eval statements with conditional function expressions';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];















status = inSection(1);
actual = eval('(function() {}); 1');
expect = 1;
addThis();

status = inSection(2);
actual = eval('(function f() {}); 2');
expect = 2;
addThis();

status = inSection(3);
actual = eval('if (true) (function() {}); 3');
expect = 3;
addThis();

status = inSection(4);
actual = eval('if (true) (function f() {}); 4');
expect = 4;
addThis();

status = inSection(5);
actual = eval('if (false) (function() {}); 5');
expect = 5;
addThis();

status = inSection(6);
actual = eval('if (false) (function f() {}); 6');
expect = 6;
addThis();

status = inSection(7);
actual = eval('switch(true) { case true: (function() {}) }; 7');
expect = 7;
addThis();

status = inSection(8);
actual = eval('switch(true) { case true: (function f() {}) }; 8');
expect = 8;
addThis();

status = inSection(9);
actual = eval('switch(false) { case false: (function() {}) }; 9');
expect = 9;
addThis();

status = inSection(10);
actual = eval('switch(false) { case false: (function f() {}) }; 10');
expect = 10;
addThis();




test();




function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}


function test()
{
  enterFunc('test');
  printBugNumber(BUGNUMBER);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
