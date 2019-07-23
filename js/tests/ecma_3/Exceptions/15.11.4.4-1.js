









































































var gTestfile = '15.11.4.4-1.js';
var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Testing Error.prototype.toString()';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var EMPTY_STRING = '';
var EXPECTED_FORMAT = 0;


status = inSection(1);
var err1 = new Error('msg1');
actual = examineThis(err1, 'msg1');
expect = EXPECTED_FORMAT;
addThis();

status = inSection(2);
var err2 = new Error(err1);
actual = examineThis(err2, err1);
expect = EXPECTED_FORMAT;
addThis();

status = inSection(3);
var err3 = new Error();
actual = examineThis(err3, EMPTY_STRING);
expect = EXPECTED_FORMAT;
addThis();

status = inSection(4);
var err4 = new Error(EMPTY_STRING);
actual = examineThis(err4, EMPTY_STRING);
expect = EXPECTED_FORMAT;
addThis();


status = inSection(5);
try
{
  eval('1=2');
}
catch(err5)
{
  actual = examineThis(err5, '.*');
}
expect = EXPECTED_FORMAT;
addThis();




test();
















function examineThis(err, msg)
{
  var pattern = err.name + '\\s*:?\\s*' + msg;
  return err.toString().search(RegExp(pattern));
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

  for (var i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
