




































var gTestfile = 'regress-452498-062.js';

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
    eval(
      '(function(){' +
      '  var x;' +
      '  this.init_by_array = function()' +
      '    x = 0;' +
      '})();'
      );
  }
  catch(ex)
  {
  }



  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
