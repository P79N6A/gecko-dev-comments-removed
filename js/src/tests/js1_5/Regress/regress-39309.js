













































var gTestfile = 'regress-39309.js';
var UBound = 0;
var BUGNUMBER = 39309;
var summary = 'Testing concatenation of string + number';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


function f(textProp, len)
{
  var i = 0;
  while (++i <= len)
  {
    var name = textProp + i;
    actual = name;
  }
}


status = inSection(1);
f('text', 1);  
expect = 'text1';
addThis();

status = inSection(2);
f('text', 100);  
expect = 'text100';
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
