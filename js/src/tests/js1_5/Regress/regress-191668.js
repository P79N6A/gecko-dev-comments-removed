















































var gTestfile = 'regress-191668.js';
var UBound = 0;
var BUGNUMBER = 191668;
var summary = 'Testing script containing <!- at internal buffer boundary';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];

var N = 512;
var j = 0;
var str = 'if (0<!-0) ++j;';

for (var i=0; i!=N; ++i)
{
  eval(str);
  str = ' ' + str;
}

status = inSection(1);
actual = j;
expect = N;
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
