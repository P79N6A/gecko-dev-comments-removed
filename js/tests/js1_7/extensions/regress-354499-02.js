




































var gTestfile = 'regress-354499-02.js';

var BUGNUMBER = 354499;
var summary = 'Iterating over Array elements';
var actual = '';
var expect = '';



test();


function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);

  expect = actual = 'No Crash';

  function get_value()
  {
    
    this[0] = 0;

    
    gc();
  }

  var counter = 2;
  Iterator.prototype.next = function()
    {
      if (counter-- <= 0) throw StopIteration;
      var a = [Math.sqrt(2), 1];
      a.__defineGetter__(1, get_value);
      return a;
    };

  for (i in [1])
    ;

  reportCompare(expect, actual, summary);

  exitFunc ('test');
}
