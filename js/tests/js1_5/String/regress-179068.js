





























































































var gTestfile = 'regress-179068.js';
var UBound = 0;
var BUGNUMBER = 179068;
var summary = 'Test that interpreter can handle string literals exceeding 64K';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var LONG_STR_SEED = "0123456789";
var N = 20 * 1024;
var str = "";



var long_str = duplicate(LONG_STR_SEED, N);
eval("str='".concat(long_str, "';"));

status = inSection(1);
actual = str.length == LONG_STR_SEED.length * N
  expect = true;
addThis();




test();




function duplicate(str, count)
{
  var tmp = new Array(count);

  while (count != 0)
    tmp[--count] = str;

  return String.prototype.concat.apply("", tmp);
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
