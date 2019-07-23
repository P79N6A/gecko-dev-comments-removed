













































var gTestfile = 'regress-83532-002.js';
var BUGNUMBER = 83532;
var summary = "Testing that we don't crash on switch case -1";
var sToEval = '';


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  
  sToEval += 'function f () {switch(1) {case -1:}};';
  sToEval += 'function g(){switch(1){case (-1):}};';
  sToEval += 'var h = function() {switch(1) {case -1:}};'
    sToEval += 'f();';
  sToEval += 'g();';
  sToEval += 'h();';
  eval(sToEval);

  reportCompare('No Crash', 'No Crash', '');

  exitFunc ('test');
}
