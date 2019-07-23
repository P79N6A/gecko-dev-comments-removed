














































var gTestfile = 'regress-94506.js';
var UBound = 0;
var BUGNUMBER = 94506;
var summary = 'Testing functions employing identifiers named "arguments"';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var TYPE_OBJECT = typeof new Object();
var arguments = 5555;



function F1(arguments)
{
  return arguments;
}



function F2()
{
  var arguments = 55;
  return arguments;
}



function F3()
{
  return arguments;
  var arguments = 555;
}



function F4()
{
  return arguments;
}








status = 'Section 1 of test';
actual = F1(5);
expect = 5;
addThis();


status = 'Section 2 of test';
actual = F2();
expect = 55;
addThis();


status = 'Section 3 of test';
actual = typeof F3();
expect = TYPE_OBJECT;
addThis();


status = 'Section 4 of test';
actual = typeof F4();
expect = TYPE_OBJECT;
addThis();



status = 'Section 5 of test';
actual = F1();
expect = undefined;
addThis();



status = 'Section 6 of test';
actual = F1(3,33,333);
expect = 3;
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
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}
