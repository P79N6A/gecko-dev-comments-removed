




































var gTestfile = 'regress-506567.js';

var BUGNUMBER = 506567;
var summary = 'Do not crash with watched variables';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  if (typeof clearInterval == 'undefined')
  {
    clearInterval = (function () {});
  }

  var obj = new Object();
  obj.test = null;
  obj.watch("test", (function(prop, oldval, newval)
    {
      if(false)
      {
        var test = newval % oldval;
        var func = (function(){clearInterval(myInterval);});
      }
    }));

  obj.test = 'null';
  print(obj.test);

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
