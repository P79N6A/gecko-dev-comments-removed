




































var gTestfile = 'regress-371802.js';

var BUGNUMBER = 371802;
var summary = 'Do not assert with group assignment';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f = (function (a,b,n){for(var [i,j]=[a,b];i<n;[i,j]=[a,b])print(i)});
  expect = 'function (a,b,n){for(var [i,j]=[a,b];(i<n);[i,j]=[a,b]){print(i);}}';
  actual = f + '';

  compareSource(expect, actual, summary);

  exitFunc ('test');
}
