














































var gTestfile = 'regress-107771.js';
var UBound = 0;
var BUGNUMBER = 107771;
var summary = "Regression test for bug 107771";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var str = '';
var k = -9999;


status = inSection(1);
str = "AAA//BBB/CCC/";
k = str.lastIndexOf('/');
actual = k;
expect = 12;

status = inSection(2);
str = str.substring(0, k);
k = str.lastIndexOf('/');
actual = k;
expect = 8;
addThis();

status = inSection(3);
str = str.substring(0, k);
k = str.lastIndexOf('/');
actual = k;
expect = 4;
addThis();

status = inSection(4);
str = str.substring(0, k);
k = str.lastIndexOf('/');
actual = k;
expect = 3;
addThis();

status = inSection(5);
str = str.substring(0, k);
k = str.lastIndexOf('/');
actual = k;
expect = -1;
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
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
