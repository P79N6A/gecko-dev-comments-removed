
















































var gTestfile = 'regress-94257.js';
var UBound = 0;
var BUGNUMBER = 94257;
var summary = "Making sure we don't crash on this code -";
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


var arr = new Array(6);
arr[1+1] = 1;
arr[1+1] += 2;


status = inSection(1);
actual = arr[1+1];
expect = 3;
addThis();

status = inSection(2);
actual = arr[1+1+1];
expect = undefined;
addThis();

status = inSection(3);
actual = arr[1];
expect = undefined;
addThis();


arr[1+2] = 'Hello';


status = inSection(4);
actual = arr[1+1+1];
expect = 'Hello';
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
