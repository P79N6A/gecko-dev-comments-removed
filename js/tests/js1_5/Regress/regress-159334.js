























































var gTestfile = 'regress-159334.js';
var UBound = 0;
var BUGNUMBER = 159334;
var summary = 'Testing script with at least 64K of different string literals';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


var N = 0xFFFE;



var long_eval = buildEval_r(0, N);


var test_sum = 0;
function f(str) { test_sum += Number(str); }
eval(long_eval);

status = inSection(1);
actual = (test_sum == N * (N - 1) / 2);
expect = true;
addThis();




test();




function buildEval_r(beginLine, endLine)
{
  var count = endLine - beginLine;

  if (count == 0)
    return "";

  if (count == 1)
    return "f('" + beginLine + "')\n";

  var middle = beginLine + (count >>> 1);
  return buildEval_r(beginLine, middle) + buildEval_r(middle, endLine);
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
