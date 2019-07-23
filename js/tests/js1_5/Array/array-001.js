












































var gTestfile = 'array-001.js';
var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Truncating arrays that have decimal property names';
var BIG_INDEX = 4294967290;
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


var arr = Array(BIG_INDEX);
arr[BIG_INDEX - 1] = 'a';
arr[BIG_INDEX - 10000] = 'b';
arr[BIG_INDEX - 0.5] = 'c';  

arr.length = BIG_INDEX - 5000;



var s = '';
for (var i in arr)
{
  s += arr[i];
}







status = inSection(1);
actual = sortThis(s);
expect = 'bc';
addThis();




test();




function sortThis(str)
{
  var chars = str.split('');
  chars = chars.sort();
  return chars.join('');
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
