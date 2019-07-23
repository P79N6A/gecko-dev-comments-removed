




































var gTestfile = 'regress-356247.js';


var BUGNUMBER = 356247;
var summary = 'Decompilation of let {} = [1] in a loop';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function() { for(let x in []) let {} = [1]; };
  expect = 'function() { for(let x in []) let [] = [1]; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  var g = eval('(' + f + ')');
  actual = g + '';
  compareSource(expect, actual, summary);

  f = function() { while(0) let {} = [1]; };
  expect = 'function() { while(0) let [] = [1]; }';
  actual = f + '';
  compareSource(expect, actual, summary);

  g = eval('(' + f + ')');
  actual = g + '';
  compareSource(expect, actual, summary);


  exitFunc ('test');
}
