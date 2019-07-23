



















































var gTestfile = 'regress-68498-001.js';
var BUGNUMBER = 68498;
var summary ='Testing that variable statement outside any eval creates'  +
  ' a DontDelete property of the global object';




var _self = this;
var actual = (delete _self);
var expect =false;



test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary); 
  reportCompare(expect, actual, summary);
  exitFunc ('test');
}
