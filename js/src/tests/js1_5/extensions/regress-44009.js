












































var gTestfile = 'regress-44009.js';
var BUGNUMBER = 44009;
var summary = "Testing that we don't crash on obj.toSource()";
var obj1 = {};
var sToSource = '';
var self = this;  




test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var obj2 = {};

  
  testThis(self);
  testThis(this);
  testThis(obj1);
  testThis(obj2);

  reportCompare('No Crash', 'No Crash', '');

  exitFunc ('test');
}



function testThis(obj)
{
  sToSource = obj.toSource();
  obj.prop = obj;
  sToSource = obj.toSource();
}
