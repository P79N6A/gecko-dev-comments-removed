




































var gTestfile = 'regress-349362.js';

var BUGNUMBER = 349362;
var summary = 'generator toString should be [object Generator]';
var actual = '';
var expect = '[object Generator]';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var y = function(){ yield 3};
  actual = y().toString();

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
