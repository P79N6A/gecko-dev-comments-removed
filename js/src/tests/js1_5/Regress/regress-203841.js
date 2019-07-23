














































var gTestfile = 'regress-203841.js';
var UBound = 0;
var BUGNUMBER = 203841;
var summary = 'Testing merged if-clauses';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
var a = 0;
var b = 0;
var c = 0;
if (a == 5, b == 6) { c = 1; }
actual = c;
expect = 0;
addThis();

status = inSection(2);
a = 5;
b = 0;
c = 0;
if (a == 5, b == 6) { c = 1; }
actual = c;
expect = 0;
addThis();

status = inSection(3);
a = 5;
b = 6;
c = 0;
if (a == 5, b == 6) { c = 1; }
actual = c;
expect = 1;
addThis();




status = inSection(4);
a = 0;
b = 6;
c = 0;
if (a = 5, b == 6) { c = 1; }
actual = c;
expect = 1;
addThis();

status = inSection(5);
c = 0;
if (1, 1 == 6) { c = 1; }
actual = c;
expect = 0;
addThis();





var x=[];

status = inSection(6); 
c = 0;
if (x[1==2]) { c = 1; }
actual = c;
expect = 0;
addThis();

status = inSection(7); 
c = 0;
if (x[1==2]=1) { c = 1; }
actual = c;
expect = 1;
addThis();

status = inSection(8); 
c = 0;
if (delete x[1==2]) { c = 1; }
actual = c;
expect = 1;
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
