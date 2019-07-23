




































































var gTestfile = 'regress-185485.js';
var UBound = 0;
var BUGNUMBER = 185485;
var summary = 'Testing |with (x) {function f() {}}| when |x.f| already exists';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];

var x = { f:0, g:0 };

with (x)
{
  f = 1;
}
status = inSection(1);
actual = x.f;
expect = 1;
addThis();

with (x)
{
  var f = 2;
}
status = inSection(2);
actual = x.f;
expect = 2;
addThis();





with (x)
{
  function f() {}
}
status = inSection(3);
actual = x.f;
expect = 2;
addThis();

status = inSection(4);
actual = typeof this.f;
expect = 'function';
addThis();







with (x)
{
  var g = function() {}
}
status = inSection(5);
actual = x.g.toString();
expect = (function () {}).toString();
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
