













































var gTestfile = '15.11.7.6-003.js';
var UBound = 0;
var BUGNUMBER = 201989;
var summary = 'Prototype of predefined error objects should be ReadOnly';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];





function testReadOnly(F)
{
  var e;
  var orig = F.prototype;
  try
  {
    F.prototype = new Object();
  }
  catch (e)
  {
  }
  return F.prototype === orig;
}


var list = [
  "Error",
  "ConversionError",
  "EvalError",
  "RangeError",
  "ReferenceError",
  "SyntaxError",
  "TypeError",
  "URIError"
  ];


for (i in list)
{
  var F = this[list[i]];

  
  if (F)
  {
    status = 'Testing ReadOnly attribute of |' + list[i] + '.prototype|';
    actual = testReadOnly(F);
    expect = true;
    addThis();
  }
}




test();




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
