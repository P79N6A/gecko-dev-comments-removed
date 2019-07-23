

















































var gTestfile = 'regress-49286.js';
var UBound = 0;
var BUGNUMBER = 49286;
var summary = 'Invoking try...catch through Function.call';
var cnErrorCaught = 'Error caught';
var cnErrorNotCaught = 'Error NOT caught';
var cnGoodSyntax = '1==2';
var cnBadSyntax = '1=2';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];


var obj = new testObject();

status = 'Section A of test: direct call of f';
actual = f.call(obj);
expect = cnErrorCaught;
addThis();

status = 'Section B of test: indirect call of f';
actual = g.call(obj);
expect = cnErrorCaught;
addThis();




test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  for (var i=0; i<UBound; i++)
  {
    reportCompare(expectedvalues[i], actualvalues[i], statusitems[i]);
  }

  exitFunc ('test');
}



function testObject()
{
  this.badSyntax = cnBadSyntax;
  this.goodSyntax = cnGoodSyntax;
}



function f()
{
  try
  {
    eval(this.badSyntax);
  }
  catch(e)
  {
    return cnErrorCaught;
  }
  return cnErrorNotCaught;
}



function g()
{
  return f.call(this);
}


function addThis()
{
  statusitems[UBound] = status;
  actualvalues[UBound] = actual;
  expectedvalues[UBound] = expect;
  UBound++;
}
