













































var gTestfile = 'switch-001.js';
var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Testing the switch statement';
var cnMatch = 'Match';
var cnNoMatch = 'NoMatch';
var status = '';
var statusitems = [ ];
var actual = '';
var actualvalues = [ ];
var expect= '';
var expectedvalues = [ ];


status = 'Section A of test';
actual = match(17, f(fInverse(17)), f, fInverse);
expect = cnMatch;
addThis();

status = 'Section B of test';
actual = match(17, 18, f, fInverse);
expect = cnNoMatch;
addThis();

status = 'Section C of test';
actual = match(1, 1, Math.exp, Math.log);
expect = cnMatch;
addThis();

status = 'Section D of test';
actual = match(1, 2, Math.exp, Math.log);
expect = cnNoMatch;
addThis();

status = 'Section E of test';
actual = match(1, 1, Math.sin, Math.cos);
expect = cnNoMatch;
addThis();




test();







function match(x, y, F, G)
{
  switch (x)
  {
  case F(G(y)):
    return cnMatch;

  default:
    return cnNoMatch;
  }
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


function f(m)
{
  return 2*(m+1);
}


function fInverse(n)
{
  return (n-2)/2;
}
