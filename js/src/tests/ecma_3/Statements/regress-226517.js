














































var gTestfile = 'regress-226517.js';
var UBound = 0;
var BUGNUMBER = 226517;
var summary = '|finally| statement should execute even after a |return|';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];






function f()
{
  var return_expression_was_calculated = false;
  try
  {
    return (return_expression_was_calculated = true);
  }
  finally
  {
    actual = return_expression_was_calculated;
  }
}


status = inSection(1);
f(); 
expect = true;
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
