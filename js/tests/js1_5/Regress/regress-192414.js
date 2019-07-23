














































var gTestfile = 'regress-192414.js';
var UBound = 0;
var BUGNUMBER = 192414;
var summary = 'Parser recursion should check stack overflow';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];






status = inSection(1);
var N = 10000;
var left = repeat_str('(1&', N);
var right = repeat_str(')', N);
var str = 'actual = '.concat(left, '1', right, ';');
try
{
  eval(str);
}
catch (e)
{
  



  actual = 1;
}
expect = 1;
addThis();




test();




function repeat_str(str, repeat_count)
{
  var arr = new Array(--repeat_count);
  while (repeat_count != 0)
    arr[--repeat_count] = str;
  return str.concat.apply(str, arr);
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
  enterFunc('test');
  printBugNumber(BUGNUMBER);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
