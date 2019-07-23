









































var UBound = 0;
var bug = '';
var summary = 'Ensuring normal function call of Error (ECMA-262 Ed.3 15.11.1.1)';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var EMPTY_STRING = '';
var EXPECTED_FORMAT = 0;


function otherScope(msg)
{
  return Error(msg);
}


status = inSection(1);
var err1 = Error('msg1');
actual = examineThis(err1, 'msg1');
expect = EXPECTED_FORMAT;
addThis();

status = inSection(2);
var err2 = otherScope('msg2');
actual = examineThis(err2, 'msg2');
expect = EXPECTED_FORMAT;
addThis();

status = inSection(3);
var err3 = otherScope();
actual = examineThis(err3, EMPTY_STRING);
expect = EXPECTED_FORMAT;
addThis();

status = inSection(4);
var err4 = eval("Error('msg4')");
actual = examineThis(err4, 'msg4');
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
  printBugNumber (bug);
  printStatus (summary);

  for (var i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
