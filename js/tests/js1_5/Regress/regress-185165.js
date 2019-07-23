













































var gTestfile = 'regress-185165.js';
var UBound = 0;
var BUGNUMBER = 185165;
var summary = 'Decompilation of "\\\\" should give "\\\\"';
var status = '';
var statusitems = [];
var actual = '';
var actualvalues = [];
var expect= '';
var expectedvalues = [];



var f1 = function() { return "\\"; }
  var s1 = f1.toString();

var f2;
eval("f2=" + s1);
var s2 = f2.toString();

status = inSection(1);
actual = s2;
expect = s1;
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
