




































var gTestfile = 'regress-381101.js';


var BUGNUMBER = 381101;
var summary = 'Decompilation of setter';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = (function(){ ({ x setter: function(){} + {} }) });
  expect = 'function(){ ({ x setter: function(){} + {} }); }';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
