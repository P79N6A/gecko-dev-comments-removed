















































var gTestfile = '15.7.4.5-1.js';
var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Testing Number.prototype.toFixed(fractionDigits)';
var cnIsRangeError = 'instanceof RangeError';
var cnNotRangeError = 'NOT instanceof RangeError';
var cnNoErrorCaught = 'NO ERROR CAUGHT...';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var testNum = 234.2040506;


status = 'Section A of test: no error intended!';
actual = testNum.toFixed(4);
expect = '234.2041';
captureThis();






















status = 'Section D of test: no error intended!';
actual =  0.00001.toFixed(2);
expect = '0.00';
captureThis();

status = 'Section E of test: no error intended!';
actual =  0.000000000000000000001.toFixed(20);
expect = '0.00000000000000000000';
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
