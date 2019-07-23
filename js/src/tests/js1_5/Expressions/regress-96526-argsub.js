

















































var gTestfile = 'regress-96526-argsub.js';
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

status = inSection(1);
actual = f(2,1,[1,2,[3,4]]);
expect = 42;
addThis();


function f(j,k)
{
  status = inSection(2);
  actual = formatArray(arguments[2]);
  expect = formatArray([1,2,[3,4]]);
  addThis();

  status = inSection(3);
  actual = formatArray(arguments[2][j]);
  expect = formatArray([3,4]);
  addThis();

  status = inSection(4);
  actual = arguments[2][j][k];
  expect = 4;
  addThis();

  status = inSection(5);
  actual = arguments[2][j][k][z];
  expect = 42;
  addThis();

  return arguments[2][j][k][z];
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
