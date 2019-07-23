




































var gTestfile = 'regress-420087.js';

var BUGNUMBER = 420087;
var summary = 'Do not assert:  PCVCAP_MAKE(sprop->shape, 0, 0) == entry->vcap';
var actual = 'No Crash';
var expect = 'No Crash';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var dict;

  for (var i = 0; i < 2; i++)
    dict = {p: 1, q: 1, p:1};

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
