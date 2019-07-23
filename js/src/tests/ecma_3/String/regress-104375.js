












































var gTestfile = 'regress-104375.js';
var UBound = 0;
var BUGNUMBER = 104375;
var summary = 'Testing string.replace() with backreferences';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];








var str = 'uid=31';
var re = /(uid=)(\d+)/;


status = inSection(1);
actual  = str.replace (re, "$1" + 15);
expect = 'uid=15';
addThis();


status = inSection(2);
actual  = str.replace (re, "$1" + '15');
expect = 'uid=15';
addThis();


status = inSection(3);
actual  = str.replace (re, "$1" + 'A15');
expect = 'uid=A15';
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


