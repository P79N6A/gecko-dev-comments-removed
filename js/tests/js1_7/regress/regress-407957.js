




































var gTestfile = 'regress-407957.js';

var BUGNUMBER = 407957;
var summary = 'Iterator is mutable.';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var obj           = {};
  var saveIterator  = Iterator;

  Iterator = obj;
  reportCompare(obj, Iterator, summary);

  exitFunc ('test');
}
