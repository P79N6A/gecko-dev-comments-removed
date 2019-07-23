




































var gTestfile = 'regress-355829-03.js';

var BUGNUMBER = 355829;
var summary = 'js_ValueToObject should return the original object if OBJ_DEFAULT_VALUE returns a primitive value';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var a = [ { valueOf: function() { return null; } } ];
  a.toLocaleString();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
