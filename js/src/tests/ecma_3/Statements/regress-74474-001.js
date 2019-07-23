















































var gTestfile = 'regress-74474-001.js';
var UBound = 0;
var BUGNUMBER = 74474;
var summary = 'Testing switch statements with duplicate labels';
var status = '';
var statusitems = [ ];
var actual = '';
var actualvalues = [ ];
var expect= '';
var expectedvalues = [ ];


status = 'Section A of test: the string literal "1" as a duplicate label';
actual = '';
switch ('1')
{
case '1':
  actual += 'a';
case '1':
  actual += 'b';
}
expect = 'ab';
addThis();


status = 'Section B of test: the numeric literal 1 as a duplicate label';
actual = '';
switch (1)
{
case 1:
  actual += 'a';
case 1:
  actual += 'b';
}
expect = 'ab';
addThis();


status = 'Section C of test: the numeric literal 1 as a duplicate label, via a function parameter';
tryThis(1);
function tryThis(x)
{
  actual = '';

  switch (x)
  {
  case x:
    actual += 'a';
  case x:
    actual += 'b';
  }
}
expect = 'ab';
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
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (var i = 0; i < UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], getStatus(i));
  }

  exitFunc ('test');
}


function getStatus(i)
{
  return statusitems[i];
}
