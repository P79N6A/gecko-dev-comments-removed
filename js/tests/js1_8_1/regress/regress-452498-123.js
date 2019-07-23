




































var gTestfile = 'regress-452498-123.js';

var BUGNUMBER = 452498;
var summary = 'TM: upvar2 regression tests';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);






  try
  {
    eval('y = (function (){y} for (x in []);');
  }
  catch(ex)
  {
  }



  (function(){for(var x = arguments in []){} function x(){}})();







  (function(){ eval("for (x in ['', {}, '', {}]) { this; }" )})();



  for each (let x in ['', '', '']) { (new function(){} )}





  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
