




















































var gTestfile = 'regress-184107.js';
var UBound = 0;
var BUGNUMBER = 184107;
var summary = 'with(...) { function f ...} should set f in the global scope';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];

var obj = {y:10};
with (obj)
{
  
  function f()
  {
    return y;
  }

  
  g = function() {return y;}
}

status = inSection(1);
actual = obj.f;
expect = undefined;
addThis();

status = inSection(2);
actual = f();
expect = obj.y;
addThis();

status = inSection(3);
actual = obj.g;
expect = undefined;
addThis();

status = inSection(4);
actual = g();
expect = obj.y;
addThis();




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
