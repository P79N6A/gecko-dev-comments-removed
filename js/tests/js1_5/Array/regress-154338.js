













































var gTestfile = 'regress-154338.js';
var UBound = 0;
var BUGNUMBER = 154338;
var summary = 'Test array.join() when separator is a variable, not a literal';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];










var x = 'Home'[0];
var y = ('Home'.split('H'))[1];


status = inSection(1);
var arr = Array('a', 'b');
actual = arr.join('H');
expect = 'aHb';
addThis();

status = inSection(2);
arr = Array('a', 'b');
actual = arr.join(x);
expect = 'aHb';
addThis();

status = inSection(3);
arr = Array('a', 'b');
actual = arr.join('ome');
expect = 'aomeb';
addThis();

status = inSection(4);
arr = Array('a', 'b');
actual = arr.join(y);
expect = 'aomeb';
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
