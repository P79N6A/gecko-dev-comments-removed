













































var gTestfile = 'regress-71107.js';
var BUGNUMBER = 71107;
var summary = 'Propagate heavyweightness back up the function-nesting chain.';


test();



function test()
{
  enterFunc ('test');
  printBugNumber(BUGNUMBER);
  printStatus (summary);
 
  var actual = outer()()();  
  var expect = 5;
  reportCompare(expect, actual, summary);

  exitFunc ('test');
}


function outer () {
  var outer_var = 5;

  function inner() {
    function way_inner() {
      return outer_var;
    }
    return way_inner;
  }
  return inner;
}
