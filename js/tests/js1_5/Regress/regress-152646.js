













































var gTestfile = 'regress-152646.js';
var UBound = 0;
var BUGNUMBER = 152646;
var summary = 'Testing expressions with large numbers of parentheses';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];









status = inSection(1);

var sLeft = '((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((((';
sLeft += sLeft;
sLeft += sLeft;
sLeft += sLeft;
sLeft += sLeft;
sLeft += sLeft;

var sRight = '))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))))';
sRight += sRight;
sRight += sRight;
sRight += sRight;
sRight += sRight;
sRight += sRight;

var sEval = 'actual = ' + sLeft + '0' + sRight;
try
{
  eval(sEval);
}
catch(e)
{
  



  actual = 0;
}
expect = 0;
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
