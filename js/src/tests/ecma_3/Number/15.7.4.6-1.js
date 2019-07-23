















































var gTestfile = '15.7.4.6-1.js';
var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Testing Number.prototype.toExponential(fractionDigits)';
var cnIsRangeError = 'instanceof RangeError';
var cnNotRangeError = 'NOT instanceof RangeError';
var cnNoErrorCaught = 'NO ERROR CAUGHT...';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var testNum = 77.1234;


status = 'Section A of test: no error intended!';
actual = testNum.toExponential(4);
expect = '7.7123e+1';
captureThis();

status = 'Section B of test: Infinity.toExponential() with out-of-range fractionDigits';
actual = Number.POSITIVE_INFINITY.toExponential(-3);
expect = 'Infinity';
captureThis();

status = 'Section C of test: -Infinity.toExponential() with out-of-range fractionDigits';
actual = Number.NEGATIVE_INFINITY.toExponential(-3);
expect = '-Infinity';
captureThis();

status = 'Section D of test: NaN.toExponential() with out-of-range fractionDigits';
actual = Number.NaN.toExponential(-3);
expect = 'NaN';
captureThis();
























test();



function captureThis()
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

  for (var i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}


function catchError(sEval)
{
  try {eval(sEval);}
  catch(e) {return isRangeError(e);}
  return cnNoErrorCaught;
}


function isRangeError(obj)
{
  if (obj instanceof RangeError)
    return cnIsRangeError;
  return cnNotRangeError;
}
