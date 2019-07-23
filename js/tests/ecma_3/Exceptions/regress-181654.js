













































var gTestfile = 'regress-181654.js';
var UBound = 0;
var BUGNUMBER = '181654';
var summary = 'Calling toString for an object derived from the Error class should be possible.';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var EMPTY_STRING = '';
var EXPECTED_FORMAT = 0;



function MyError( msg )
{
  this.message = msg;
}
MyError.prototype = new Error();
MyError.prototype.name = "MyError";


status = inSection(1);
var err1 = new MyError('msg1');
actual = examineThis(err1, 'msg1');
expect = EXPECTED_FORMAT;
addThis();

status = inSection(2);
var err2 = new MyError(String(err1));
actual = examineThis(err2, err1);
expect = EXPECTED_FORMAT;
addThis();

status = inSection(3);
var err3 = new MyError();
actual = examineThis(err3, EMPTY_STRING);
expect = EXPECTED_FORMAT;
addThis();

status = inSection(4);
var err4 = new MyError(EMPTY_STRING);
actual = examineThis(err4, EMPTY_STRING);
expect = EXPECTED_FORMAT;
addThis();


status = inSection(5);
try
{
  throw new MyError("thrown");
}
catch(err5)
{
  actual = examineThis(err5, "thrown");
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
