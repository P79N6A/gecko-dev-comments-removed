














































var gTestfile = 'regress-191633.js';
var UBound = 0;
var BUGNUMBER = 191633;
var summary = 'Testing script with huge number of comments';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
actual = false; 
var s = repeat_str("//\n", 40000); 
eval(s + "actual = true;");
expect = true;
addThis();




test();




function repeat_str(str, repeat_count)
{
  var arr = new Array(repeat_count);

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
