



































































var gTestfile = 'scope-003.js';
var UBound = 0;
var BUGNUMBER = '(none)';
var summary = 'Testing scope with nested functions';
var statprefix = 'Section ';
var statsuffix = ' of test -';
var self = this; 
var cnGlobal = self.toString();
var cnObject = (new Object).toString();
var statusitems = [];
var actualvalues = [];
var expectedvalues = [];


function a()
{
  function b()
  {
    capture(this.toString());
  }

  this.c = function()
    {
      capture(this.toString());
      b();
    }

  b();
}


var obj = new a();  
obj.c();            



expectedvalues[0] = cnGlobal;
expectedvalues[1] = cnObject;
expectedvalues[2] = cnGlobal;




test();




function capture(val)
{
  actualvalues[UBound] = val;
  statusitems[UBound] = getStatus(UBound);
  UBound++;
}


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


function getStatus(i)
{
  return statprefix + i + statsuffix;
}
