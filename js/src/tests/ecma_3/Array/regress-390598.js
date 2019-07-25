






































var BUGNUMBER = 390598;
var summary = 'Override inherited length of Array-like object';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 

  function F() {} 
  F.prototype = []; 

  
  expect = 10;
  var x = new F(); 

  print('x = new F(); x instanceof Array: ' + (x instanceof Array));

  x.length = expect;
  actual = x.length;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
