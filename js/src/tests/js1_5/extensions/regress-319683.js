




































var gTestfile = 'regress-319683.js';

var BUGNUMBER = 319683;
var summary = 'Do not crash in call_enumerate';
var actual = 'No Crash';
var expect = 'No Crash';
printBugNumber(BUGNUMBER);
printStatus (summary);

function crash(){
  function f(){
    var x;
    function g(){
      x=1; 
    }
  }

  
  f.__proto__={};

  
  f();
} 

crash();

reportCompare(expect, actual, summary);
