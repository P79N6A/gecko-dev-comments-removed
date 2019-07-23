














































var gTestfile = 'regress-202678-001.js';
var UBound = 0;
var BUGNUMBER = 202678;
var summary = 'Testing nested function scope capture';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];
var self = this;


function myFunc()
{
  var hidden = 'aaa';
  insideFunc();

  if (!self.runOnce)
  {
    var hidden = 'bbb';
    self.outSideFunc = insideFunc;
    self.runOnce = true;
  }
  else
  {
    var hidden = 'ccc';
  }


  function insideFunc()
  {
    actual = hidden;
  }
}



status = inSection(1);
myFunc();  
expect = 'aaa';
addThis();

status = inSection(2);
outSideFunc();  
expect = 'bbb';
addThis();

status = inSection(3);
myFunc();      
expect = 'aaa';
addThis();

status = inSection(4);
outSideFunc();  
expect = 'bbb'; 
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
