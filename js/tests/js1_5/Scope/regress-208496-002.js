

















































var gTestfile = 'regress-208496-002.js';
var UBound = 0;
var BUGNUMBER = 208496;
var summary = 'Testing |with (f)| inside the definition of |function f()|';
var STATIC_VALUE = 'read the static property';
var status = '';
var statusitems = [];
var actual = '(TEST FAILURE)';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


function f(par)
{
  with(f)
  {
    actual = par;
  }

  return par;
}
f.par = STATIC_VALUE;


status = inSection(1);
f('abc'); 
expect = STATIC_VALUE;
addThis();


status = inSection(2);
actual = f('abc');
expect = 'abc';
addThis();

status = inSection(3);
f(111 + 222); 
expect = STATIC_VALUE;
addThis();


status = inSection(4);
actual = f(111 + 222);
expect = 333;
addThis();





function g(par)
{
  with(g)
  {
    var x = par;
    actual = x;
  }

  return par;
}
g.par = STATIC_VALUE;


status = inSection(5);
g('abc'); 
expect = STATIC_VALUE;
addThis();


status = inSection(6);
actual = g('abc');
expect = 'abc';
addThis();

status = inSection(7);
g(111 + 222); 
expect = STATIC_VALUE;
addThis();


status = inSection(8);
actual = g(111 + 222);
expect = 333;
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
