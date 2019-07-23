




































var gTestfile = 'regress-349482-02.js';

var BUGNUMBER = 349482;
var summary = 'Decompiling try/catch in with() should not crash';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { with({})     { try{}catch(e){} } }
  print(f.toString());

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
