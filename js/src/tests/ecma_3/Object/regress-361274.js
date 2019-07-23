




































var gTestfile = 'regress-361274.js';

var BUGNUMBER = 361274;
var summary = 'Embedded nulls in property names';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var x='123'+'\0'+'456';
  var y='123'+'\0'+'789';
  var a={};
  a[x]=1;
  a[y]=2;

  reportCompare(1, a[x], summary + ': 123\\0456');
  reportCompare(2, a[y], summary + ': 123\\0789');

  exitFunc ('test');
}
