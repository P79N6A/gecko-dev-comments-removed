




































var gTestfile = 'regress-452498-129.js';

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





  ({ set x x () { for(x in function(){}){}}  })




try
{
    (function (){ eval("(function(){delete !function(){}});"); })();
}
catch(ex)
{
}




  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
