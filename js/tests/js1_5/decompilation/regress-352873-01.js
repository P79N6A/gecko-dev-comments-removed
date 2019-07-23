




































var gTestfile = 'regress-352873-01.js';

var BUGNUMBER = 352873;
var summary = 'decompilation of nested |try...catch| with |with|';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  var f;

  f = function() { try { with({}) { try { } finally { return; }  }  } finally { } }
  expect = 'function() { try { with({}) { try { } finally { return; }  }  } finally { } }';
  actual = f + '';
  compareSource(expect, actual, summary);

  expect = actual = 'No Crash';
  f();
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
