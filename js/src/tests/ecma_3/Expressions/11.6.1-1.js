

















































var gTestfile = '11.6.1-1.js';
var UBound = 0;
var BUGNUMBER = 196290;
var summary = 'Testing left-associativity of the + operator';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


status = inSection(1);
actual = 1 + 1 + 'px';
expect = '2px';
addThis();

status = inSection(2);
actual = 'px' + 1 + 1;
expect = 'px11';
addThis();

status = inSection(3);
actual = 1 + 1 + 1 + 'px';
expect = '3px';
addThis();

status = inSection(4);
actual = 1 + 1 + 'a' + 1 + 1 + 'b';
expect = '2a11b';
addThis();




status = inSection(5);
actual = sumThese(1, 1, 'a', 1, 1, 'b');
expect = '2a11b';
addThis();

status = inSection(6);
actual = sumThese(new Number(1), new Number(1), 'a');
expect = '2a';
addThis();

status = inSection(7);
actual = sumThese('a', new Number(1), new Number(1));
expect = 'a11';
addThis();




test();




function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}



















function sumThese()
{
  var sEval = '';
  var arg;
  var i;

  var L = arguments.length;
  for (i=0; i<L; i++)
  {
    arg = arguments[i];
    if (typeof arg === 'string')
      arg = quoteThis(arg);

    if (i < L-1)
      sEval += arg + ' + ';
    else
      sEval += arg;
  }

  return eval(sEval);
}


function quoteThis(x)
{
  return '"' + x + '"';
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
