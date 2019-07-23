























































var gTestfile = 'scope-002.js';
var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Testing that functions are scoped statically, not dynamically';
var self = this;  
var status = '';
var statusitems = [ ];
var actual = '';
var actualvalues = [ ];
var expect= '';
var expectedvalues = [ ];









status = 'Section A of test';
var a = 1;
var f = function () {return a;};
var obj = {a:2};
with (obj)
{
  actual = f();
}
expect = 1;
addThis();






status = 'Section B of test';
var a = 1;
var obj = {a:2};
with (obj)
{
  var f = function () {return a;};
  actual = f();
}
expect = 2;
addThis();






status = 'Section C of test';
var a = 1;
var obj = {a:2};
with (obj)
{
  var f = function () {return a;};
}
actual = f();
expect = 2;
addThis();





status = 'Section D of test';
var a = 1;
var obj = {a:2, obj:{a:3}};
with (obj)
{
  with (obj)
  {
    var f = function () {return a;};
  }
}
actual = f();
expect = 3;
addThis();






status = 'Section E of test';
var a = 1;
var obj = {a:2};
with (obj)
{
  var f = function () {return a;};
}
delete obj;
actual = f();
expect = 2;
addThis();






status = 'Section F of test';
var a = 1;
var obj = {a:2};
with (obj)
{
  var f = function () {return a;};
}
delete obj;
var obj = {a:3};
with (obj)
{
  actual = f();
}
expect = 2;  
addThis();






status = 'Section G of test';
var a = 1;
var obj = {a:2};
with (obj)
{
  var f = function () {return a;};
}
actual = String([obj.hasOwnProperty('f'), self.hasOwnProperty('f')]);
expect = String([false, true]);
addThis();






status = 'Section H of test';
var a = 1;
var obj = {a:2};
with (obj)
{
  var f = function () {return a;};
}
actual = String(['f' in obj, 'f' in self]);
expect = String([false, true]);
addThis();




test();



function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
  resetTestVars();
}


function resetTestVars()
{
  delete a;
  delete obj;
  delete f;
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
