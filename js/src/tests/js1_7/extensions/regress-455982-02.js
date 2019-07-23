




































var gTestfile = 'regress-455982-02.js';

var BUGNUMBER = 455982;
var summary = 'Do not assert with JIT: with generator as getter';
var actual = 'No Crash';
var expect = 'No Crash';

printBugNumber(BUGNUMBER);
printStatus (summary);

jit(true);

for (let i=0;i<5;++i) 
  this["y" + i] = (function(){});

this.__defineGetter__('e', function (x2) { yield; });

[1 for each (a in this) for (b in {p:1,q:2,r:3})];

jit(false);
 
reportCompare(expect, actual, summary);
