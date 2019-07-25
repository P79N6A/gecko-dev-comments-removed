





































var BUGNUMBER = 351116;
var summary = 'formal parameter and inner function have same name';
var actual = 'No Crash';
var expect = 'No Crash';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function (s) { function s() { } };

  function g(s) { function s() { } }

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
