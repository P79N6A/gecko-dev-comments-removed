















































var gTestfile = 'regress-101964.js';
var UBound = 0;
var BUGNUMBER = 101964;
var summary = 'Performance: truncating even very large arrays should be fast!';
var BIG = 10000000;
var LITTLE = 10;
var FAST = 50; 
var MSG_FAST = 'Truncation took less than ' + FAST + ' ms';
var MSG_SLOW = 'Truncation took ';
var MSG_MS = ' ms';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];



status = inSection(1);
var arr = Array(BIG);
var start = new Date();
arr.length = LITTLE;
actual = elapsedTime(start);
expect = FAST;
addThis();




test();




function elapsedTime(startTime)
{
  return new Date() - startTime;
}


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = isThisFast(actual);
  expectedvalues[UBound] = isThisFast(expect);
  UBound++;
}


function isThisFast(ms)
{
  if (ms <= FAST)
    return MSG_FAST;
  return MSG_SLOW + ms + MSG_MS;
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
