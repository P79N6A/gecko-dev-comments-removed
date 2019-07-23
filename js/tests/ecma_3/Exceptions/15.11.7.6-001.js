











































var UBound = 0;
var bug = 201989;
var summary = 'Prototype of predefined error objects should be DontEnum';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];





function testDontEnum(F)
{
  var proto = F.prototype;

  for (var prop in F)
  {
    if (F[prop] === proto)
      return false;
  }
  return true;
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
    status = 'Testing DontEnum attribute of |' + list[i] + '.prototype|';
    actual = testDontEnum(F);
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
  printBugNumber(bug);
  printStatus(summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
