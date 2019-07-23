













































var gTestfile = 'regress-83532-001.js';
var BUGNUMBER = 83532;
var summary = "Testing that we don't crash on switch case -1";


test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  
  function f () {switch(1) {case -1:}}
  function g(){switch(1){case (-1):}}
  var h = function() {switch(1) {case -1:}}
  f();
  g();
  h();
  reportCompare('No Crash', 'No Crash', '');

  exitFunc ('test');
}
