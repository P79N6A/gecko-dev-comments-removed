

















































var gTestfile = 'regress-96526-delelem.js';
var UBound = 0;
var BUGNUMBER = 96526;
var summary = 'Testing "use" and "set" ops on expressions like a[i][j][k]';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];

var z='magic';
Number.prototype.magic=42;
f(2,1,[-1,0,[1,2,[3,4]]]);

function f(j,k,a)
{
  status = inSection(1);
  actual = formatArray(a[2]);
  expect = formatArray([1,2,[3,4]]);
  addThis();

  status = inSection(2);
  actual = formatArray(a[2][j]);
  expect = formatArray([3,4]);
  addThis();

  status = inSection(3);
  actual = a[2][j][k];
  expect = 4;
  addThis();

  status = inSection(4);
  actual = a[2][j][k][z];
  expect = 42;
  addThis();

  delete a[2][j][k];

  status = inSection(5);
  actual = formatArray(a[2][j]);
  expect = '[3, ,]';
  addThis();
}




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
