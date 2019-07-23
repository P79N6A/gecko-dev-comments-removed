















































var gTestfile = 'regress-102725.js';
var UBound = 0;
var BUGNUMBER = 102725;
var summary = 'Testing converting numbers to strings';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];





status = inSection(1);
foo = (new Date()).getTime();
actual = foo.toString();
expect = foo.toString();
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
