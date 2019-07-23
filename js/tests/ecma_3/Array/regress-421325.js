




































var gTestfile = 'regress-421325.js';

var BUGNUMBER = 421325;
var summary = 'Dense Arrays and holes';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  Array.prototype[1] = 'bar';

  var a = []; 
  a[0]='foo'; 
  a[2] = 'baz'; 
  expect = 'foo,bar,baz';
  actual = a + '';

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
