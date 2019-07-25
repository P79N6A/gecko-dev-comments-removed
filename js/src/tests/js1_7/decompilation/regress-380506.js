






































var BUGNUMBER = 380506;
var summary = 'Decompilation of nested-for and for-if comprehensions';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var f = function (){return [i*i for(i in [0]) if (i%2)]};
  expect = 'function (){return [i*i for(i in [0]) if (i%2)];}';
  actual = f + '';
  compareSource(expect, actual, summary);

  f = function (){return [i*j for(i in [0]) for (j in [1])]};
  expect = 'function (){return [i*j for(i in [0]) for (j in [1])];}';
  actual = f + '';
  compareSource(expect, actual, summary);

  exitFunc ('test');
}
