























































var gTestfile = 'regress-101488.js';
var UBound = 0;
var BUGNUMBER = 101488;
var summary = 'Try assigning arr.length = new Number(n)';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var arr = [];


status = inSection(1);
arr = Array();
tryThis('arr.length = new Number(1);');
actual = arr.length;
expect = 1;
addThis();

status = inSection(2);
arr = Array(5);
tryThis('arr.length = new Number(1);');
actual = arr.length;
expect = 1;
addThis();

status = inSection(3);
arr = Array();
tryThis('arr.length = new Number(17);');
actual = arr.length;
expect = 17;
addThis();

status = inSection(4);
arr = Array(5);
tryThis('arr.length = new Number(17);');
actual = arr.length;
expect = 17;
addThis();






status = inSection(5);
arr = new Array();
tryThis('arr.length = new Number(1);');
actual = arr.length;
expect = 1;
addThis();

status = inSection(6);
arr = new Array(5);
tryThis('arr.length = new Number(1);');
actual = arr.length;
expect = 1;
addThis();

arr = new Array();
tryThis('arr.length = new Number(17);');
actual = arr.length;
expect = 17;
addThis();

status = inSection(7);
arr = new Array(5);
tryThis('arr.length = new Number(17);');
actual = arr.length;
expect = 17;
addThis();




test();




function tryThis(s)
{
  try
  {
    eval(s);
  }
  catch(e)
  {
    
  }
}


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
